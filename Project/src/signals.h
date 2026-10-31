#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/* Set to 1 inside the SIGINT handler; checked cooperatively by the
 * scanner and worker threads so a Ctrl+C finishes the file currently
 * in flight instead of corrupting it mid-copy. */
extern volatile sig_atomic_t stop_requested;

/* Installs a SIGINT handler via sigaction(). CO3: signals as an IPC/
 * notification mechanism between the kernel and a user-space process. */
void install_signal_handlers(void);

#endif /* SIGNALS_H */
