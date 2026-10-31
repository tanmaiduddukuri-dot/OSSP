#define _POSIX_C_SOURCE 200809L

#include "signals.h"

#include <string.h>
#include <unistd.h>

volatile sig_atomic_t stop_requested = 0;

/* Only async-signal-safe calls are allowed in here (no printf!) - write()
 * on a raw fd is one of the few safe ways to report the interrupt. */
static void handle_sigint(int signo) {
    (void)signo;
    stop_requested = 1;
    const char msg[] = "\n[SIGINT] Interrupt received - finishing in-flight files, then stopping...\n";
    ssize_t ignored = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    (void)ignored;
}

void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* deliberately no SA_RESTART: blocking calls should
                       * notice the interrupt rather than silently resume */
    sigaction(SIGINT, &sa, NULL);
}
