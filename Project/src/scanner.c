#define _POSIX_C_SOURCE 200809L

#include "scanner.h"
#include "utils.h"
#include "signals.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int build_task_list(const char *source_dir, const char *backup_dir,
                     TaskQueue *queue, SyncStats *stats) {
    if (!path_exists(source_dir)) {
        fprintf(stderr, "Error: source directory '%s' does not exist.\n", source_dir);
        return -1;
    }

    /* mkdir() the backup directory the first time we see it. Directories
     * are created here, single-threaded, so two workers can never race
     * to mkdir() the same path. */
    int created = ensure_directory(backup_dir);
    if (created < 0) {
        return -1;
    }
    if (created == 1) {
        printf("[MKDIR]     %s\n", backup_dir);
        stats->dirs_created++;
    }

    DIR *dir = opendir(source_dir);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while (!stop_requested && (entry = readdir(dir)) != NULL) {
        /* Skip the "." and ".." entries every directory contains. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024];
        char dst_path[1024];
        build_path(src_path, sizeof(src_path), source_dir, entry->d_name);
        build_path(dst_path, sizeof(dst_path), backup_dir, entry->d_name);

        /* CO5: stat() reads the file's inode - size, type, and timestamps
         * all live in the inode, not the directory entry itself. */
        struct stat entry_stat;
        if (stat(src_path, &entry_stat) != 0) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            /* Recurse so nested folders are backed up too. */
            build_task_list(src_path, dst_path, queue, stats);
        } else if (S_ISREG(entry_stat.st_mode)) {
            /* Hand the file off to the worker pool instead of copying it
             * here - this thread's job is purely to discover work. */
            queue_push(queue, src_path, dst_path);
        }
        /* Other file types (symlinks, devices, etc.) are intentionally
         * left alone - out of scope for this project. */
    }

    closedir(dir);
    return 0;
}
