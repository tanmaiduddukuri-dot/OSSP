#ifndef SYNC_ENGINE_H
#define SYNC_ENGINE_H

typedef enum {
    FILE_COPIED,   /* file did not exist in backup, was copied           */
    FILE_UPDATED,  /* file existed but had changed, was re-copied        */
    FILE_SKIPPED,  /* file existed and was identical, left untouched     */
    FILE_ERROR     /* a system call failed while handling this file      */
} FileSyncResult;

/*
 * Compares src_path against dst_path (existence, size, modification time)
 * and copies the file with low-level I/O (open/read/write/close) only if
 * something changed. Reports the outcome via FileSyncResult.
 */
FileSyncResult sync_file(const char *src_path, const char *dst_path);

#endif /* SYNC_ENGINE_H */
