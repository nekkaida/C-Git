# Compiler settings
CC = gcc

# Base flags
CFLAGS_BASE = -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -g -Iinclude

# Warning flags (comprehensive)
CFLAGS_WARN = -Wconversion -Wimplicit-fallthrough -Wformat=2 \
              -Wformat-security -Werror=format-security \
              -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes \
              -Wmissing-prototypes -Wredundant-decls -Wnested-externs \
              -Wdouble-promotion -Wnull-dereference

# Security hardening flags (OpenSSF recommended)
CFLAGS_SECURITY = -fstack-protector-strong -fstack-clash-protection \
                  -D_FORTIFY_SOURCE=2 -fPIE \
                  -fno-strict-overflow -fno-strict-aliasing \
                  -fno-delete-null-pointer-checks \
                  -ftrivial-auto-var-init=zero

# Architecture-specific hardening
UNAME_M := $(shell uname -m 2>/dev/null || echo unknown)
ifeq ($(UNAME_M),x86_64)
    CFLAGS_ARCH = -fcf-protection=full
else ifeq ($(UNAME_M),aarch64)
    CFLAGS_ARCH = -mbranch-protection=standard
else
    CFLAGS_ARCH =
endif

# Combined flags
CFLAGS = $(CFLAGS_BASE) $(CFLAGS_WARN) $(CFLAGS_SECURITY) $(CFLAGS_ARCH)

# Linker flags with security hardening (OpenSSF recommended)
LDFLAGS = -lz -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack -Wl,-z,nodlopen

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
TESTS_DIR = tests
OBJ_DIR = build
BIN_DIR = .

# Test binaries
TEST_BINS = $(BIN_DIR)/test_sha1 $(BIN_DIR)/test_validation $(BIN_DIR)/test_object

# Library objects (for linking with tests)
LIB_OBJS = $(CORE_OBJS) $(UTILS_OBJS)

# Source files
CORE_SRCS = $(CORE_DIR)/sha1.c $(CORE_DIR)/object.c $(CORE_DIR)/tree.c $(CORE_DIR)/index.c
UTILS_SRCS = $(UTILS_DIR)/validation.c $(UTILS_DIR)/error.c
COMMANDS_SRCS = $(COMMANDS_DIR)/init.c \
                $(COMMANDS_DIR)/hash_object.c \
                $(COMMANDS_DIR)/cat_file.c \
                $(COMMANDS_DIR)/ls_tree.c \
                $(COMMANDS_DIR)/write_tree.c \
                $(COMMANDS_DIR)/commit_tree.c \
                $(COMMANDS_DIR)/add.c \
                $(COMMANDS_DIR)/status.c \
                $(COMMANDS_DIR)/log.c
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
test-all: test test-unit
	@echo "Running all tests..."
	@cd test-repo && \
	TREE_HASH=$$(../$(TARGET) write-tree) && \
	../$(TARGET) commit-tree $$TREE_HASH -m "Initial commit" && \
	../$(TARGET) ls-tree $$TREE_HASH
	@echo "All tests passed!"

# Unit tests
test-unit: $(TEST_BINS)
	@echo "Running unit tests..."
	@echo ""
	@echo "=== SHA-1 Tests ==="
	@./test_sha1
	@echo ""
	@echo "=== Validation Tests ==="
	@./test_validation
	@echo ""
	@echo "=== Object Tests ==="
	@./test_object
	@echo ""
	@echo "All unit tests completed!"

# Build test binaries
$(BIN_DIR)/test_sha1: $(TESTS_DIR)/test_sha1.c $(LIB_OBJS)
	@echo "Building test_sha1..."
	$(CC) $(CFLAGS_BASE) -I$(TESTS_DIR) -o $@ $< $(LIB_OBJS) $(LDFLAGS)

$(BIN_DIR)/test_validation: $(TESTS_DIR)/test_validation.c $(LIB_OBJS)
	@echo "Building test_validation..."
	$(CC) $(CFLAGS_BASE) -I$(TESTS_DIR) -o $@ $< $(LIB_OBJS) $(LDFLAGS)

$(BIN_DIR)/test_object: $(TESTS_DIR)/test_object.c $(LIB_OBJS)
	@echo "Building test_object..."
	$(CC) $(CFLAGS_BASE) -I$(TESTS_DIR) -o $@ $< $(LIB_OBJS) $(LDFLAGS)

# Clean tests
clean-tests:
	$(RM) $(TEST_BINS) 2>/dev/null || true

# Run with Valgrind to check for memory leaks
valgrind: $(BIN_DIR)/$(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) init

# Build with AddressSanitizer
asan: clean
	$(MAKE) CFLAGS="$(CFLAGS_BASE) -fsanitize=address,undefined -fno-omit-frame-pointer -g" \
		LDFLAGS="-lz -fsanitize=address,undefined"

# Build with MemorySanitizer (requires clang)
msan: clean
	$(MAKE) CC=clang CFLAGS="$(CFLAGS_BASE) -fsanitize=memory -fno-omit-frame-pointer -g" \
		LDFLAGS="-lz -fsanitize=memory"

# Build with ThreadSanitizer
tsan: clean
	$(MAKE) CFLAGS="$(CFLAGS_BASE) -fsanitize=thread -g" LDFLAGS="-lz -fsanitize=thread"

# Static analysis with cppcheck
cppcheck:
	@echo "Running cppcheck static analysis..."
	cppcheck --enable=all --std=c11 --error-exitcode=1 \
		--suppress=missingIncludeSystem \
		-I include $(SRC_DIR)

# Static analysis with clang-analyzer (scan-build)
scan-build: clean
	scan-build --status-bugs $(MAKE) all

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
	@echo "  test        - Run basic integration tests"
	@echo "  test-unit   - Run unit tests (TAP format)"
	@echo "  test-all    - Run all tests (integration + unit)"
	@echo "  clean-tests - Remove test binaries"
	@echo "  valgrind    - Run with memory leak detection"
	@echo "  asan        - Build with AddressSanitizer + UBSan"
	@echo "  msan        - Build with MemorySanitizer (requires clang)"
	@echo "  tsan        - Build with ThreadSanitizer"
	@echo "  cppcheck    - Run cppcheck static analysis"
	@echo "  scan-build  - Run clang static analyzer"
	@echo "  format      - Format code with clang-format"
	@echo "  install     - Install to /usr/local/bin"
	@echo "  info        - Show build configuration"
	@echo "  help        - Show this help message"

.PHONY: all clean clean-tests check_deps install test test-unit test-all valgrind asan msan tsan cppcheck scan-build format info help
