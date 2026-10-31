#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Joins dir and name into dest as "dir/name", safely bounded by dest_size. */
void build_path(char *dest, size_t dest_size, const char *dir, const char *name);

/* Wraps access(path, F_OK). Returns 1 if the path exists, 0 otherwise. */
int path_exists(const char *path);

/*
 * Ensures 'path' exists as a directory, creating it with mkdir() if needed.
 * Returns 0 if it already existed, 1 if it was just created, -1 on error.
 */
int ensure_directory(const char *path);

#endif /* UTILS_H */
