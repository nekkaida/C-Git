# Compiler settings
CC = gcc

# Base flags
CFLAGS_BASE = -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -g -Iinclude

# Security hardening flags
CFLAGS_SECURITY = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE \
                  -Wformat -Wformat-security -Wformat=2 -fno-strict-overflow

# Combined flags
CFLAGS = $(CFLAGS_BASE) $(CFLAGS_SECURITY)

# Linker flags with security hardening
LDFLAGS = -lz -pie -Wl,-z,relro,-z,now

# Platform detection
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Windows)
    # Windows-specific settings (MinGW/Cygwin)
    RM = del /Q
    MKDIR = mkdir
    MKDIR_P = if not exist
    TARGET = main.exe
else
    # Unix-like settings
    RM = rm -f
    MKDIR = mkdir -p
    MKDIR_P = mkdir -p
    TARGET = main
endif

# Directories
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
UTILS_DIR = $(SRC_DIR)/utils
COMMANDS_DIR = $(SRC_DIR)/commands
OBJ_DIR = build
BIN_DIR = .

# Source files
CORE_SRCS = $(CORE_DIR)/sha1.c $(CORE_DIR)/object.c $(CORE_DIR)/tree.c
UTILS_SRCS = $(UTILS_DIR)/validation.c $(UTILS_DIR)/error.c
COMMANDS_SRCS = $(COMMANDS_DIR)/init.c \
                $(COMMANDS_DIR)/hash_object.c \
                $(COMMANDS_DIR)/cat_file.c \
                $(COMMANDS_DIR)/ls_tree.c \
                $(COMMANDS_DIR)/write_tree.c \
                $(COMMANDS_DIR)/commit_tree.c
MAIN_SRC = $(SRC_DIR)/main_refactored.c

ALL_SRCS = $(CORE_SRCS) $(UTILS_SRCS) $(COMMANDS_SRCS) $(MAIN_SRC)

# Object files
CORE_OBJS = $(patsubst $(CORE_DIR)/%.c,$(OBJ_DIR)/core/%.o,$(CORE_SRCS))
UTILS_OBJS = $(patsubst $(UTILS_DIR)/%.c,$(OBJ_DIR)/utils/%.o,$(UTILS_SRCS))
COMMANDS_OBJS = $(patsubst $(COMMANDS_DIR)/%.c,$(OBJ_DIR)/commands/%.o,$(COMMANDS_SRCS))
MAIN_OBJ = $(OBJ_DIR)/main_refactored.o

ALL_OBJS = $(CORE_OBJS) $(UTILS_OBJS) $(COMMANDS_OBJS) $(MAIN_OBJ)

# Create build directories
ifeq ($(UNAME_S),Windows)
$(shell if not exist build\core mkdir build\core 2>nul)
$(shell if not exist build\utils mkdir build\utils 2>nul)
$(shell if not exist build\commands mkdir build\commands 2>nul)
else
$(shell $(MKDIR) $(OBJ_DIR)/core $(OBJ_DIR)/utils $(OBJ_DIR)/commands 2>/dev/null)
endif

# Default target
all: $(BIN_DIR)/$(TARGET)

# Link
$(BIN_DIR)/$(TARGET): $(ALL_OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete!"

# Compile core modules
$(OBJ_DIR)/core/%.o: $(CORE_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile utils modules
$(OBJ_DIR)/utils/%.o: $(UTILS_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile command modules
$(OBJ_DIR)/commands/%.o: $(COMMANDS_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main
$(MAIN_OBJ): $(MAIN_SRC)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Check dependencies
check_deps:
	@echo "Checking dependencies..."
	@echo "#include <zlib.h>" | $(CC) -E - >/dev/null 2>&1 || (echo "Error: zlib development files not found" && exit 1)
	@echo "All dependencies satisfied"

# Install (to /usr/local/bin by default)
install: $(BIN_DIR)/$(TARGET)
	install -m 755 $(BIN_DIR)/$(TARGET) /usr/local/bin/

# Cleanup
clean:
	@echo "Cleaning build files..."
	$(RM) -r $(OBJ_DIR) 2>/dev/null || true
	$(RM) $(BIN_DIR)/$(TARGET) 2>/dev/null || true
	@echo "Clean complete!"

# Testing
test: $(BIN_DIR)/$(TARGET)
	@echo "Running basic tests..."
ifeq ($(UNAME_S),Windows)
	@if not exist test-repo mkdir test-repo
	@cd test-repo && ..\$(TARGET) init && echo Hello, world! > test.txt && ..\$(TARGET) hash-object -w test.txt && ..\$(TARGET) write-tree
else
	@mkdir -p test-repo && cd test-repo && ../$(TARGET) init && echo "Hello, world!" > test.txt && ../$(TARGET) hash-object -w test.txt && ../$(TARGET) write-tree
endif
	@echo "Basic tests passed!"

# More comprehensive testing
test-all: test
	@echo "Running all tests..."
	@cd test-repo && \
	TREE_HASH=$$(../$(TARGET) write-tree) && \
	../$(TARGET) commit-tree $$TREE_HASH -m "Initial commit" && \
	../$(TARGET) ls-tree $$TREE_HASH
	@echo "All tests passed!"

# Run with Valgrind to check for memory leaks
valgrind: $(BIN_DIR)/$(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) init

# Code formatting (if clang-format is available)
format:
	find $(SRC_DIR) include -name "*.c" -o -name "*.h" | xargs clang-format -i

# Show build info
info:
	@echo "Build Configuration:"
	@echo "  Platform: $(UNAME_S)"
	@echo "  Compiler: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  Target: $(TARGET)"
	@echo "  Sources: $(words $(ALL_SRCS)) files"
	@echo "  Objects: $(words $(ALL_OBJS)) files"

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build the project (default)"
	@echo "  clean       - Remove build files"
	@echo "  check_deps  - Check if dependencies are installed"
	@echo "  test        - Run basic tests"
	@echo "  test-all    - Run comprehensive tests"
	@echo "  valgrind    - Run with memory leak detection"
	@echo "  format      - Format code with clang-format"
	@echo "  install     - Install to /usr/local/bin"
	@echo "  info        - Show build configuration"
	@echo "  help        - Show this help message"

.PHONY: all clean check_deps install test test-all valgrind format info help
