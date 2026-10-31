CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Isrc -pthread
SRC_DIR = src
OBJ_DIR = obj
BIN     = backupsync

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)

test: all
	chmod +x test/run_tests.sh
	./test/run_tests.sh

.PHONY: all clean test
