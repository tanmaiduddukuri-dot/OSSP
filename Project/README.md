# Linux File Backup and Synchronization System Using System Calls

**Course:** Operating Systems and Systems Programming (25CS2104E) — 2026–27, Term-I
**Section 13, Team 2**
D Sri Tanmai (2520080042) · V. Akshitha (2520080003) · K. Aasritha Sai Lakshmi (2520090006)

A command-line tool that backs up and synchronizes files between a source directory
and a backup directory, using nothing but Linux/POSIX system calls. Directories are
scanned recursively; a pool of worker threads copies changed files concurrently;
large files use memory-mapped I/O; a signal handler makes Ctrl+C safe; and a final
`fork()`/`exec()` step verifies the result — the same mechanism your shell uses to
run any command.

## How it works, end to end

1. **Scan** the source directory with `opendir()` / `readdir()` / `closedir()`,
   creating matching backup directories with `mkdir()` as it goes, and enqueuing
   one job per regular file it finds instead of copying immediately.
2. **Copy concurrently**: a fixed pool of worker threads (`pthread_create`) pulls
   jobs off that queue and copies each file, protected by a mutex where they touch
   shared state.
3. **Compare** each file against its backup counterpart using `stat()` (size and
   modification time) before copying — unchanged files are skipped entirely.
4. **Copy** changed files either with a classic `open()`/`read()`/`write()`/`close()`
   loop (small files) or via `mmap()` (files ≥ 1 MB), letting the kernel page the
   data in on demand instead of an explicit read loop.
5. **Interrupt safely**: `Ctrl+C` is caught by a `SIGINT` handler; in-flight files
   finish, then the tool stops and reports a partial summary instead of dying
   mid-copy.
6. **Verify**: after copying, the tool `fork()`s a child that `exec()`s `diff -rq`
   on the two trees, piping its output back to the parent, and reports whether the
   backup is byte-for-byte identical to the source.

## Mapping to the course outcomes

| CO | Subtopic covered | Where in the code |
|----|---|---|
| **CO1** — syscall interface, shell as a user-space program, tracing a command from typing to completion | The verification step (`verify.c`) does exactly what a shell does for every command you type: `fork()` a child, `exec()` replaces its image with `/usr/bin/diff`, the kernel runs it in user mode, and the parent's `waitpid()` observes it finish. | `src/verify.c` |
| **CO2** — `fork`, `exec`, `wait`, `exit` | Same verification step: `fork()`, `execlp()`, `waitpid()`, and `_exit()` on the exec-failure path. | `src/verify.c` |
| **CO3** — pipes, signals | An anonymous `pipe()` carries the child's `diff` output back to the parent (`dup2` redirects the child's stdout/stderr into it). A `SIGINT` handler (`sigaction`) sets a flag that the scanner and workers check cooperatively for a graceful shutdown. | `src/verify.c`, `src/signals.c` |
| **CO4** — paging, address translation, page faults, demand paging, copy-on-write | Files ≥ 1 MB are copied via `mmap()` (source mapped `MAP_PRIVATE`/copy-on-write, backup mapped `MAP_SHARED`) and a single `memcpy()`; the kernel faults pages in from disk on first touch instead of an explicit read loop. | `src/sync_engine.c` (`copy_file_mmap`) |
| **CO5** — inodes, directory entries, read/write/seek semantics, file-I/O syscalls | `stat()`/`fstat()` read each file's inode (size, mtime, permissions — printed alongside every copied file); `readdir()` walks directory entries; `lseek()` is used explicitly before the read/write loop. | `src/scanner.c`, `src/sync_engine.c`, `src/worker.c` |
| **CO6** — POSIX threads, mutexes, condition variables, counting semaphores, race conditions | A producer/consumer pipeline: the scanner thread produces file-copy jobs onto a queue guarded by a mutex + condition variable (`task_queue.c`); a pool of worker threads (`pthread_create`) consumes them; a counting semaphore (`sem_t`) throttles how many copies run concurrently independent of thread count; shared statistics are updated under a mutex to avoid races. | `src/task_queue.c`, `src/worker.c`, `src/main.c` |

## Project layout

```
backup-sync-project/
├── src/
│   ├── main.c          # CLI entry point: wires threads, queue, signals, verify together
│   ├── scanner.c/.h     # Recursive directory walk -> produces file-copy tasks (CO5)
│   ├── task_queue.c/.h  # Thread-safe producer/consumer queue (CO6: mutex + condvar)
│   ├── worker.c/.h      # Worker thread pool: consumes tasks, copies files (CO6)
│   ├── sync_engine.c/.h # Per-file compare + copy: read/write and mmap paths (CO4, CO5)
│   ├── verify.c/.h      # fork()+exec("diff")+pipe()+wait() verification (CO1, CO2, CO3)
│   ├── signals.c/.h     # SIGINT handling for graceful shutdown (CO3)
│   └── utils.c/.h       # Path building, access()/mkdir() helpers
├── test/
│   └── run_tests.sh    # Functional tests: copy, skip, update, large-file mmap path
├── Makefile
└── README.md
```

## Build

Requires `gcc`, `make`, and `pthreads` on Linux (developed/tested on Ubuntu). `diff`
(from `diffutils`) must be on `PATH` for the verification step — it ships by default
on virtually every Linux distribution.

```bash
make          # builds ./backupsync
make clean    # removes build artifacts
```

## Run

```bash
./backupsync <source_directory> <backup_directory> [--no-verify]
```

Example:

```bash
./backupsync ~/Documents/project_files ~/Backups/project_files
```

- The backup directory is created automatically if it doesn't exist.
- Run the same command again any time — only new or changed files are copied;
  everything else is reported as `SKIPPED`.
- Pass `--no-verify` to skip the closing `fork`/`exec`/`diff` verification step.
- Press `Ctrl+C` mid-run to see the graceful-shutdown path: in-flight files finish,
  then the tool reports how far it got.

## Test

```bash
make test
# or directly:
./test/run_tests.sh
```

It checks that:
- Run 1 copies every file (fresh backup).
- Run 2 skips every file (nothing changed).
- Run 3 updates only the one file modified in between.
- The final backup directory is byte-identical to the source (`diff -r`).
- A 3 MB file (above the mmap threshold) is copied correctly via `mmap()` and is
  byte-identical to the source.

## Observing the system calls (optional, for the demo/report)

On your own Ubuntu machine you can trace exactly which system calls run:

```bash
strace -f -e trace=open,openat,read,write,close,stat,mkdir,access,mmap,fork,execve,wait4,rt_sigaction \
    ./backupsync source_dir backup_dir
```

The `-f` flag follows the child process spawned for verification, so you'll see the
`fork()`/`execve("/usr/bin/diff", ...)`/`wait4()` sequence as well as the `mmap()`
calls for any large file. This is good evidence for the report/demo.

## Notes / scope

- Handles regular files and directories (recursively). Symlinks, device files, etc.
  are intentionally skipped — out of scope per the project brief.
- Change detection uses size **and** modification time; it does not hash contents.
- 4 worker threads and a 2-slot copy semaphore are fixed constants in `main.c`
  (`NUM_WORKERS`, `MAX_CONCURRENT_COPIES`) — easy to point out and tweak live during
  a demo to show the concurrency model.
- Signal delivery in a multithreaded process can technically land on any thread
  with the signal unblocked; this project keeps the demo simple by installing the
  handler before spawning threads and treating `stop_requested` as a shared flag
  everyone polls, rather than doing per-thread signal masking.
- This is a user-space command-line tool only — no GUI, web, or database layer,
  in line with the project restrictions.
