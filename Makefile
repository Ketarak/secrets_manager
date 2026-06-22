# Makefile - Secrets Manager (passmgr)

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -g $(shell pkg-config --cflags libsodium 2>/dev/null || echo "")
LDFLAGS = $(shell pkg-config --libs libsodium 2>/dev/null || echo "-lsodium")

# Project directories
SRC_DIR = src
OBJ_DIR = obj
TARGET = passmgr

# Native messaging host installation variables
HOST_NAME = passmgr
MANIFEST_SRC = passmgr.json.template
MANIFEST_DIR = $(HOME)/.mozilla/native-messaging-hosts
MANIFEST_DEST = $(MANIFEST_DIR)/$(HOST_NAME).json
BINARY_ABS_PATH = $(abspath $(TARGET))

# Sources and objects list
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean debug sanitize install-native help

all: $(TARGET)

# Link the main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Target with AddressSanitizer and UndefinedBehaviorSanitizer enabled
sanitize: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDFLAGS += -fsanitize=address,undefined
sanitize: clean $(TARGET)

# Install Firefox Native Messaging host manifest
install-native: $(TARGET)
	@mkdir -p $(MANIFEST_DIR)
	@sed "s|TARGET_PATH|$(BINARY_ABS_PATH)|g" $(MANIFEST_SRC) > $(MANIFEST_DEST)
	@echo "[+] Native messaging manifest installed at: $(MANIFEST_DEST)"
	@echo "    Absolute binary path: $(BINARY_ABS_PATH)"

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(TARGET) test_crypto

# Run all tests (unit and integration)
test: $(TARGET)
	@echo "=== Building Unit Tests ==="
	$(CC) $(CFLAGS) tests/test_crypto.c src/crypto.c -o test_crypto $(LDFLAGS)
	@echo "=== Running Unit Tests ==="
	./test_crypto
	@echo "=== Running Integration Tests ==="
	./tests/integration.sh
	@rm -f test_crypto

# Help menu
help:
	@echo "Available commands:"
	@echo "  make                : Compile the project"
	@echo "  make sanitize       : Compile with AddressSanitizer"
	@echo "  make test           : Run unit and integration tests"
	@echo "  make install-native : Install Firefox Native Messaging manifest"
	@echo "  make clean          : Remove build objects and binary"