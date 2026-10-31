#define _POSIX_C_SOURCE 200809L

#include "worker.h"
#include "sync_engine.h"
#include "signals.h"

#include <stdio.h>
#include <sys/stat.h>

void *worker_thread(void *arg) {
    WorkerArgs *w = (WorkerArgs *)arg;
    FileTask task;

    while (queue_pop(w->queue, &task) == 0) {
        /* CO6: a counting semaphore throttles how many copies run truly
         * concurrently, independent of how many threads exist - e.g. 4
         * worker threads but only 2 files being copied at once, to cap
         * open file descriptors / disk I/O contention. */
        sem_wait(w->copy_slots);
        FileSyncResult result = sync_file(task.src, task.dst);
        sem_post(w->copy_slots);

        /* CO5: report the inode backing this file, to make the
         * inode-vs-directory-entry distinction visible in the output. */
        struct stat st;
        long inode = (stat(task.src, &st) == 0) ? (long)st.st_ino : -1;

        pthread_mutex_lock(w->stats_lock);
        switch (result) {
            case FILE_COPIED:
                printf("[COPIED]    %-40s (inode %ld) [thread %d]\n", task.src, inode, w->id);
                w->stats->copied++;
                break;
            case FILE_UPDATED:
                printf("[UPDATED]   %-40s (inode %ld) [thread %d]\n", task.src, inode, w->id);
                w->stats->updated++;
                break;
            case FILE_SKIPPED:
                printf("[SKIPPED]   %-40s (unchanged)         [thread %d]\n", task.src, w->id);
                w->stats->skipped++;
                break;
            case FILE_ERROR:
                fprintf(stderr, "[ERROR]     %s [thread %d]\n", task.src, w->id);
                break;
        }
        pthread_mutex_unlock(w->stats_lock);

        if (stop_requested) {
            break; /* finish the file already in flight, then stop */
        }
    }

    return NULL;
}
