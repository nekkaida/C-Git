# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lcurl -lz

# Target executable
TARGET = main

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Check if curl and zlib are available
check_deps:
	@echo "Checking dependencies..."
	@which curl >/dev/null || (echo "Error: curl not found" && exit 1)
	@echo "#include <zlib.h>" | $(CC) -E - >/dev/null 2>&1 || (echo "Error: zlib development files not found" && exit 1)
	@echo "#include <curl/curl.h>" | $(CC) -E - >/dev/null 2>&1 || (echo "Error: libcurl development files not found" && exit 1)
	@echo "All dependencies satisfied"

# Install (to /usr/local/bin by default)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Cleanup
clean:
	rm -f $(OBJ) $(TARGET)

# Testing
test: $(TARGET)
	@echo "Running basic tests..."
	@mkdir -p test-repo && cd test-repo && \
	../$(TARGET) init && \
	echo "Hello, world!" > test.txt && \
	../$(TARGET) hash-object -w test.txt && \
	../$(TARGET) write-tree

# More comprehensive testing
test-all: test
	@echo "Running all tests..."
	@cd test-repo && \
	TREE_HASH=$$(../$(TARGET) write-tree) && \
	../$(TARGET) commit-tree $$TREE_HASH -m "Initial commit" && \
	../$(TARGET) ls-tree $$TREE_HASH

# Run with Valgrind to check for memory leaks
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) init

.PHONY: all clean install check_deps test test-all valgrind