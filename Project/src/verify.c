#define _POSIX_C_SOURCE 200809L

#include "verify.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int verify_backup(const char *source_dir, const char *backup_dir) {
    int pipefd[2];
    if (pipe(pipefd) != 0) { /* CO3: anonymous pipe for parent<->child IPC */
        perror("pipe");
        return -1;
    }

    pid_t pid = fork(); /* CO2: fork() clones this process */
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        /* --- child --- */
        close(pipefd[0]);            /* child only writes */
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        /* CO1/CO2: replace this process's image with /usr/bin/diff -
         * the same fork()+exec() pair a shell performs for every
         * command you type. The kernel takes over from here, running
         * diff in user mode until it calls exit(). */
        execlp("diff", "diff", "-rq", source_dir, backup_dir, (char *)NULL);

        /* Only reached if execlp() itself failed (e.g. diff not installed). */
        perror("execlp(\"diff\")");
        _exit(127);
    }

    /* --- parent --- */
    close(pipefd[1]); /* parent only reads */

    printf("\n--- Verification: fork() + exec(\"diff\") + pipe() ---\n");
    char buf[4096];
    ssize_t n;
    int got_output = 0;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        got_output = 1;
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0); /* CO2: wait() for the child to finish/exit */

    if (!got_output && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("No differences found - backup matches source exactly.\n");
    }
    printf("-----------------------------------------------------\n");

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}
