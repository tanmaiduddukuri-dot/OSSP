#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "scanner.h"
#include "task_queue.h"
#include "worker.h"
#include "signals.h"
#include "verify.h"

#define NUM_WORKERS 4            /* CO6: size of the worker thread pool   */
#define MAX_CONCURRENT_COPIES 2  /* CO6: counting-semaphore throttle      */

static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <source_directory> <backup_directory> [--no-verify]\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *source_dir = argv[1];
    const char *backup_dir = argv[2];
    int do_verify = 1;

    if (argc == 4) {
        if (strcmp(argv[3], "--no-verify") == 0) {
            do_verify = 0;
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    install_signal_handlers(); /* CO3: Ctrl+C triggers a graceful stop */

    printf("==========================================\n");
    printf(" Linux File Backup & Synchronization Tool\n");
    printf("==========================================\n");
    printf("Source directory : %s\n", source_dir);
    printf("Backup directory : %s\n", backup_dir);
    printf("Worker threads   : %d (max %d concurrent copies)\n\n", NUM_WORKERS, MAX_CONCURRENT_COPIES);

    TaskQueue queue;
    queue_init(&queue);

    SyncStats stats = {0, 0, 0, 0};
    pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

    sem_t copy_slots;
    sem_init(&copy_slots, 0, MAX_CONCURRENT_COPIES);

    /* Start the consumer pool first - they'll block on the (empty) queue
     * until the scanner (producer, running on this thread) hands them
     * work. Classic producer/consumer with a mutex + condition variable. */
    pthread_t threads[NUM_WORKERS];
    WorkerArgs worker_args[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        worker_args[i].id = i + 1;
        worker_args[i].queue = &queue;
        worker_args[i].stats = &stats;
        worker_args[i].stats_lock = &stats_lock;
        worker_args[i].copy_slots = &copy_slots;
        pthread_create(&threads[i], NULL, worker_thread, &worker_args[i]);
    }

    int scan_status = build_task_list(source_dir, backup_dir, &queue, &stats);
    queue_mark_finished(&queue); /* tells idle workers there's nothing more coming */

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&copy_slots);
    pthread_mutex_destroy(&stats_lock);
    queue_destroy(&queue);

    if (scan_status != 0) {
        fprintf(stderr, "\nBackup process failed.\n");
        return EXIT_FAILURE;
    }

    if (stop_requested) {
        printf("\nStopped early after Ctrl+C - summary reflects work completed so far.\n");
    }

    printf("\n------------------------------------------\n");
    printf(" Backup Summary\n");
    printf("------------------------------------------\n");
    printf("  Files copied        : %d\n", stats.copied);
    printf("  Files updated       : %d\n", stats.updated);
    printf("  Files skipped       : %d (unchanged)\n", stats.skipped);
    printf("  Directories created : %d\n", stats.dirs_created);
    printf("------------------------------------------\n");

    if (do_verify && !stop_requested) {
        int diff_status = verify_backup(source_dir, backup_dir);
        if (diff_status == 0) {
            printf("Verification: PASSED (backup matches source)\n");
        } else if (diff_status > 0) {
            printf("Verification: DIFFERENCES FOUND (see diff output above)\n");
        } else {
            printf("Verification: could not run (is 'diff' installed?)\n");
        }
    }

    return EXIT_SUCCESS;
}
