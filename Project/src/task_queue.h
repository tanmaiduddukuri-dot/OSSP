#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <pthread.h>

/* One unit of work: copy src to dst. */
typedef struct {
    char src[1024];
    char dst[1024];
} FileTask;

typedef struct TaskNode {
    FileTask task;
    struct TaskNode *next;
} TaskNode;

/*
 * A simple linked-list queue shared between the scanner thread (producer)
 * and the worker threads (consumers). CO6: guarded by a mutex, with a
 * condition variable so consumers block instead of busy-waiting when the
 * queue is empty.
 */
typedef struct {
    TaskNode *head;
    TaskNode *tail;
    int finished; /* set once the producer has enqueued everything */
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} TaskQueue;

void queue_init(TaskQueue *q);
void queue_push(TaskQueue *q, const char *src, const char *dst);

/* Blocks until a task is available or the queue is finished+empty.
 * Returns 0 with *out filled in, or -1 if there is nothing left to do. */
int queue_pop(TaskQueue *q, FileTask *out);

/* Called once by the producer after the last queue_push(). Wakes every
 * consumer blocked in queue_pop() so they can notice there is no more work. */
void queue_mark_finished(TaskQueue *q);

void queue_destroy(TaskQueue *q);

#endif /* TASK_QUEUE_H */
