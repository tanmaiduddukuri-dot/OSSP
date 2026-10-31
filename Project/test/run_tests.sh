#!/bin/bash
# Simple functional test for backupsync.
# Run via `make test`, or directly as ./test/run_tests.sh from the project root.
set -e

BIN=./backupsync
SRC=test_source
DST=test_backup

echo "=== Setting up test environment ==="
rm -rf "$SRC" "$DST"
mkdir -p "$SRC/subdir"
echo "This is file one"   > "$SRC/file1.txt"
echo "This is file two"   > "$SRC/file2.txt"
echo "A nested file"      > "$SRC/subdir/nested.txt"

echo
echo "=== Run 1: initial backup (expect 3 files copied) ==="
$BIN "$SRC" "$DST"

echo
echo "=== Run 2: nothing changed (expect 3 files skipped) ==="
$BIN "$SRC" "$DST"

echo
echo "=== Modifying file1.txt ==="
sleep 1   # ensure a distinct mtime
echo "This is file one - MODIFIED" > "$SRC/file1.txt"

echo
echo "=== Run 3: expect file1 updated, others skipped ==="
$BIN "$SRC" "$DST"

echo
echo "=== Verifying backup matches source ==="
if diff -r "$SRC" "$DST" > /dev/null; then
    echo "PASS: backup directory matches source directory"
else
    echo "FAIL: backup and source differ"
    exit 1
fi

echo
echo "=== Cleaning up small-file test directories ==="
rm -rf "$SRC" "$DST"

echo
echo "=== Run 4: large file, exercises the mmap() copy path (CO4) ==="
BIGSRC=test_big_source
BIGDST=test_big_backup
rm -rf "$BIGSRC" "$BIGDST"
mkdir -p "$BIGSRC"
head -c 3145728 /dev/urandom > "$BIGSRC/large.bin"   # 3 MB, above the 1 MB mmap threshold
$BIN "$BIGSRC" "$BIGDST"

if diff "$BIGSRC/large.bin" "$BIGDST/large.bin" > /dev/null; then
    echo "PASS: mmap-copied large file is byte-identical to the source"
else
    echo "FAIL: mmap-copied large file differs from the source"
    exit 1
fi
rm -rf "$BIGSRC" "$BIGDST"

echo
echo "All tests completed."
