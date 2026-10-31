#ifndef SCANNER_H
#define SCANNER_H

#include "task_queue.h"

/* Running totals for the backup session, printed as a summary at the end.
 * dirs_created is only touched by the single scanning thread, so it needs
 * no lock; copied/updated/skipped are updated by worker threads under
 * stats_lock (see worker.h). */
typedef struct {
    int copied;
    int updated;
    int skipped;
    int dirs_created;
} SyncStats;

/*
 * Recursively walks source_dir using opendir()/readdir()/closedir(),
 * mirrors its structure into backup_dir (creating directories with
 * mkdir() as needed), and pushes one FileTask per regular file onto
 * 'queue' for the worker thread pool to process (see worker.h).
 *
 * Returns 0 on success, -1 on a fatal error (e.g. source_dir missing).
 */
int build_task_list(const char *source_dir, const char *backup_dir,
                     TaskQueue *queue, SyncStats *stats);

#endif /* SCANNER_H */
