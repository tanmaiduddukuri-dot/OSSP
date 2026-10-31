#include "task_queue.h"

#include <stdlib.h>
#include <string.h>

void queue_init(TaskQueue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->finished = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void queue_push(TaskQueue *q, const char *src, const char *dst) {
    TaskNode *node = malloc(sizeof(TaskNode));
    if (node == NULL) {
        return; /* out of memory - drop the task rather than crash */
    }

    strncpy(node->task.src, src, sizeof(node->task.src) - 1);
    node->task.src[sizeof(node->task.src) - 1] = '\0';
    strncpy(node->task.dst, dst, sizeof(node->task.dst) - 1);
    node->task.dst[sizeof(node->task.dst) - 1] = '\0';
    node->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->tail != NULL) {
        q->tail->next = node;
    } else {
        q->head = node;
    }
    q->tail = node;
    pthread_cond_signal(&q->not_empty); /* wake one waiting worker */
    pthread_mutex_unlock(&q->lock);
}

int queue_pop(TaskQueue *q, FileTask *out) {
    pthread_mutex_lock(&q->lock);

    while (q->head == NULL && !q->finished) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->head == NULL) {
        /* Producer is done and nothing is left - this worker can exit. */
        pthread_mutex_unlock(&q->lock);
        return -1;
    }

    TaskNode *node = q->head;
    *out = node->task;
    q->head = node->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }

    pthread_mutex_unlock(&q->lock);
    free(node);
    return 0;
}

void queue_mark_finished(TaskQueue *q) {
    pthread_mutex_lock(&q->lock);
    q->finished = 1;
    pthread_cond_broadcast(&q->not_empty); /* wake every idle worker */
    pthread_mutex_unlock(&q->lock);
}

void queue_destroy(TaskQueue *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
}
