#ifndef VERIFY_H
#define VERIFY_H

/*
 * Verifies the backup by running the equivalent of a shell command,
 * `diff -rq <source_dir> <backup_dir>`, entirely through system calls:
 *
 *   CO1 - this is exactly what a shell does when you type a command:
 *         fork a child, exec() the program, and let the kernel run it
 *         in user mode until it exits.
 *   CO2 - fork(), execlp(), and waitpid() drive the whole thing.
 *   CO3 - an anonymous pipe() carries diff's output back to the parent
 *         instead of letting it print directly to the terminal.
 *
 * Returns 0 if diff found no differences, a positive exit status if it
 * found differences, or -1 if diff could not be run at all.
 */
int verify_backup(const char *source_dir, const char *backup_dir);

#endif /* VERIFY_H */
