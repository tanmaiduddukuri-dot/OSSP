#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

void build_path(char *dest, size_t dest_size, const char *dir, const char *name) {
    snprintf(dest, dest_size, "%s/%s", dir, name);
}

int path_exists(const char *path) {
    /* access() is the standard POSIX way to check existence/permissions
     * without opening the file. F_OK just tests that the path resolves. */
    return access(path, F_OK) == 0;
}

int ensure_directory(const char *path) {
    if (path_exists(path)) {
        return 0;
    }

    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return -1;
    }

    return 1;
}
