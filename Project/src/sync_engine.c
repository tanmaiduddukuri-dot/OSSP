#define _POSIX_C_SOURCE 200809L

#include "sync_engine.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#define COPY_BUFFER_SIZE 8192
#define MMAP_THRESHOLD (1024 * 1024) /* files >= 1 MB copy via mmap() */

/*
 * Decides whether dst_path needs to be (re)written.
 * Returns: 1 = needs copy, 0 = unchanged, -1 = error.
 */
static int needs_sync(const char *src_path, const char *dst_path) {
    struct stat src_stat, dst_stat;

    /* stat() gives us size and modification time - the two properties
     * the project uses to decide whether a file has changed. */
    if (stat(src_path, &src_stat) != 0) {
        perror("stat (source)");
        return -1;
    }

    if (access(dst_path, F_OK) != 0) {
        return 1; /* no backup copy yet */
    }

    if (stat(dst_path, &dst_stat) != 0) {
        perror("stat (backup)");
        return -1;
    }

    if (src_stat.st_size != dst_stat.st_size) return 1;
    if (src_stat.st_mtime != dst_stat.st_mtime) return 1;

    return 0;
}

/*
 * CO4 - Virtual memory: instead of an explicit read()/write() loop, map
 * both files straight into this process's address space and let memcpy()
 * move the bytes. The kernel doesn't actually load any of it up front -
 * touching src_map for the first time triggers a page fault, the kernel
 * transparently pulls that page in from disk (demand paging), and the
 * MMU/TLB translate every subsequent access in that page for free. The
 * source mapping is MAP_PRIVATE (copy-on-write semantics: writes would be
 * invisible to the underlying file), the destination is MAP_SHARED so our
 * writes land back in the backup file.
 */
static int copy_file_mmap(int src_fd, int dst_fd, size_t size) {
    void *src_map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (src_map == MAP_FAILED) {
        perror("mmap (source)");
        return -1;
    }

    if (ftruncate(dst_fd, (off_t)size) != 0) {
        perror("ftruncate");
        munmap(src_map, size);
        return -1;
    }

    void *dst_map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dst_fd, 0);
    if (dst_map == MAP_FAILED) {
        perror("mmap (backup)");
        munmap(src_map, size);
        return -1;
    }

    memcpy(dst_map, src_map, size); /* page faults happen here, transparently */
    msync(dst_map, size, MS_SYNC);

    munmap(src_map, size);
    munmap(dst_map, size);
    return 0;
}

/* Small/medium files: the classic explicit read()/write() loop. */
static int copy_file_read_write(int src_fd, int dst_fd) {
    /* CO5 - file offsets/seek semantics: open() already positions the
     * offset at 0, but we rewind explicitly with lseek() so this
     * function is correct even if that ever changes (e.g. a future
     * resume-from-offset feature). */
    if (lseek(src_fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return -1;
    }

    char buffer[COPY_BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(src_fd, buffer, COPY_BUFFER_SIZE)) > 0) {
        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t written = write(dst_fd, buffer + total_written, bytes_read - total_written);
            if (written < 0) {
                perror("write");
                return -1;
            }
            total_written += written;
        }
    }

    if (bytes_read < 0) {
        perror("read");
        return -1;
    }

    return 0;
}

/* Opens both files, picks a copy strategy based on size, and preserves
 * the source's modification time on the backup afterwards. */
static int copy_file_contents(const char *src_path, const char *dst_path) {
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        perror("open (source)");
        return -1;
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) != 0) {
        perror("fstat");
        close(src_fd);
        return -1;
    }

    /* Preserve the source file's permission bits on the backup copy.
     * O_RDWR (not O_WRONLY) because the mmap() path needs to read back
     * what it just mapped for writing. */
    int dst_fd = open(dst_path, O_RDWR | O_CREAT | O_TRUNC, src_stat.st_mode & 0777);
    if (dst_fd < 0) {
        perror("open (backup)");
        close(src_fd);
        return -1;
    }

    int rc;
    if (src_stat.st_size == 0) {
        rc = 0; /* O_TRUNC already produced an empty file - nothing to copy */
    } else if (src_stat.st_size >= MMAP_THRESHOLD) {
        rc = copy_file_mmap(src_fd, dst_fd, (size_t)src_stat.st_size);
    } else {
        rc = copy_file_read_write(src_fd, dst_fd);
    }

    close(src_fd);
    close(dst_fd);

    if (rc == 0) {
        /* Stamp the backup copy with the source's mtime so that a future
         * run with an unchanged file compares equal and gets skipped. */
        struct timespec times[2];
        times[0] = src_stat.st_atim;
        times[1] = src_stat.st_mtim;
        utimensat(AT_FDCWD, dst_path, times, 0);
    }

    return rc;
}

FileSyncResult sync_file(const char *src_path, const char *dst_path) {
    int already_existed = (access(dst_path, F_OK) == 0);
    int status = needs_sync(src_path, dst_path);

    if (status < 0) {
        return FILE_ERROR;
    }
    if (status == 0) {
        return FILE_SKIPPED;
    }
    if (copy_file_contents(src_path, dst_path) != 0) {
        return FILE_ERROR;
    }

    return already_existed ? FILE_UPDATED : FILE_COPIED;
}
