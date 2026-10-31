#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <semaphore.h>

#include "task_queue.h"
#include "scanner.h" /* SyncStats */

/* Everything one worker thread needs. Several threads share the same
 * queue, stats block, mutex, and semaphore - only 'id' is per-thread. */
typedef struct {
    int id;
    TaskQueue *queue;
    SyncStats *stats;
    pthread_mutex_t *stats_lock;
    sem_t *copy_slots; /* counting semaphore bounding concurrent copies */
} WorkerArgs;

/* Thread entry point: pop tasks until the queue is drained, copying each
 * one and updating the shared SyncStats under stats_lock. */
void *worker_thread(void *arg);

#endif /* WORKER_H */
