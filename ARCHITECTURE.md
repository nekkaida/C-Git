# C-Git Architecture

This document describes the architecture of C-Git, an educational Git implementation in C.

## Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLI Layer                                │
│                    (src/main_refactored.c)                       │
│         Parses arguments, dispatches to command handlers         │
└─────────────────────────────┬───────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────┐
│                       Command Layer                              │
│                    (src/commands/*.c)                            │
│   init, add, status, log, hash-object, cat-file, ls-tree,       │
│   write-tree, commit-tree                                        │
└─────────────────────────────┬───────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────┐
│                        Core Layer                                │
│                     (src/core/*.c)                               │
│           object.c, tree.c, index.c, sha1.c                      │
└─────────────────────────────┬───────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────┐
│                       Utils Layer                                │
│                     (src/utils/*.c)                              │
│              validation.c, error.c                               │
└─────────────────────────────┬───────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────┐
│                    External Libraries                            │
│                        zlib (compression)                        │
└─────────────────────────────────────────────────────────────────┘
```

## Directory Structure

```
C-Git/
├── include/                    # Public header files
│   ├── git_common.h           # Common definitions, error codes, limits
│   ├── git_sha1.h             # SHA-1 hashing interface
│   ├── git_object.h           # Git object (blob/tree/commit) interface
│   ├── git_tree.h             # Tree builder interface
│   ├── git_index.h            # Index/staging area interface
│   └── git_validation.h       # Input validation interface
│
├── src/
│   ├── main_refactored.c      # Entry point, command dispatch
│   │
│   ├── core/                  # Core Git functionality
│   │   ├── sha1.c            # SHA-1 implementation
│   │   ├── object.c          # Object storage (compress/decompress/read/write)
│   │   ├── tree.c            # Tree object building
│   │   └── index.c           # Index/staging area
│   │
│   ├── commands/              # Command implementations
│   │   ├── init.c            # git init
│   │   ├── add.c             # git add
│   │   ├── status.c          # git status
│   │   ├── log.c             # git log
│   │   ├── hash_object.c     # git hash-object
│   │   ├── cat_file.c        # git cat-file
│   │   ├── ls_tree.c         # git ls-tree
│   │   ├── write_tree.c      # git write-tree
│   │   └── commit_tree.c     # git commit-tree
│   │
│   └── utils/                 # Utility functions
│       ├── validation.c      # Input validation
│       └── error.c           # Error handling (thread-safe)
│
├── tests/                     # Unit tests
│   ├── test_framework.h      # TAP-compatible test framework
│   ├── test_sha1.c           # SHA-1 tests
│   ├── test_validation.c     # Validation tests
│   └── test_object.c         # Object tests
│
├── fuzz/                      # Fuzz testing
│   ├── README.md             # Fuzzing instructions
│   ├── fuzz_sha1.c           # SHA-1 fuzzer
│   ├── fuzz_object.c         # Object parser fuzzer
│   └── fuzz_validation.c     # Validation fuzzer
│
└── Makefile                   # Build system
```

## Module Descriptions

### Core Modules

#### sha1.c - SHA-1 Hashing
- Custom SHA-1 implementation (no OpenSSL dependency)
- Incremental hashing support via context
- Hex string conversion utilities

```c
// Key functions:
void git_sha1_init(git_sha1_ctx *ctx);
void git_sha1_update(git_sha1_ctx *ctx, const void *data, size_t len);
void git_sha1_final(unsigned char digest[20], git_sha1_ctx *ctx);
int git_sha1_to_hex(const unsigned char *sha1, char *hex, size_t size);
int git_sha1_from_hex(const char *hex, unsigned char *sha1);
```

#### object.c - Git Object Storage
- Handles blob, tree, commit, tag objects
- zlib compression/decompression
- Object path calculation (.git/objects/XX/YYYY...)

```c
// Key functions:
git_object* git_object_new(git_object_t type, size_t size);
void git_object_free(git_object *obj);
int git_object_hash(const void *data, size_t size, git_object_t type, unsigned char sha1[20]);
int git_object_write(git_object *obj);
int git_object_read(const char *sha1_hex, git_object **out);
```

#### tree.c - Tree Object Builder
- Git-compatible tree sorting (directories with trailing '/')
- Recursive directory traversal
- Tree serialization

```c
// Key functions:
git_tree_builder* git_tree_builder_new(void);
void git_tree_builder_free(git_tree_builder *builder);
int git_tree_builder_add(git_tree_builder *builder, const char *mode, const char *name, const unsigned char sha1[20]);
int git_tree_builder_write(git_tree_builder *builder, unsigned char sha1[20]);
int git_tree_write_from_directory(const char *path, unsigned char sha1[20]);
```

#### index.c - Git Index (Staging Area)
- Binary index format compatible with real Git
- Entry management (add, remove, find)
- Tree conversion

```c
// Key functions:
git_index* git_index_new(void);
void git_index_free(git_index *index);
int git_index_read(git_index *index, const char *path);
int git_index_write(git_index *index, const char *path);
int git_index_add_from_workdir(git_index *index, const char *path);
int git_index_write_tree(git_index *index, unsigned char sha1[20]);
int git_index_read_tree(git_index *index, const char *tree_sha1_hex);
```

### Utility Modules

#### validation.c - Input Validation
- SHA-1 hex string validation
- Path traversal prevention
- Mode validation
- Safe string operations

```c
// Key functions:
int git_validate_sha1_hex(const char *hex);
int git_validate_safe_path(const char *path);
int git_validate_mode(const char *mode);
int git_safe_strncpy(char *dst, const char *src, size_t size);
int git_safe_path_join(char *dst, size_t size, const char *dir, const char *file);
```

#### error.c - Error Handling
- Thread-safe error state (C11 _Thread_local)
- Error code and message storage

```c
// Key functions:
void git_error_set(git_error_t code, const char *message);
git_error_t git_error_last_code(void);
const char* git_error_last(void);
void git_error_clear(void);
```

## Data Flow

### Adding a File (git add)

```
1. cmd_add() receives file path
2. git_index_new() creates index structure
3. git_index_read() loads existing .git/index
4. git_index_add_from_workdir():
   a. stat() file to get metadata
   b. Read file content
   c. git_object_new(GIT_OBJ_BLOB, size) creates blob
   d. git_object_write() compresses and stores in .git/objects
   e. git_index_add() adds entry with SHA-1
5. git_index_write() saves updated .git/index
```

### Creating a Commit

```
1. git_index_write_tree() creates tree from index
   a. Sort entries by path
   b. Serialize tree format: "<mode> <name>\0<20-byte-sha>"
   c. git_object_write() stores tree
2. cmd_commit_tree() creates commit:
   a. Build commit content: tree, parent, author, committer, message
   b. git_object_new(GIT_OBJ_COMMIT, size)
   c. git_object_write() stores commit
3. Update .git/refs/heads/<branch> with commit SHA
```

## Security Model

### Input Validation (Defense in Depth)

```
Layer 1: Command handlers validate arguments
Layer 2: git_validate_* functions check format
Layer 3: Size limits prevent resource exhaustion
Layer 4: Bounds checking in all operations
```

### Size Limits

| Limit | Value | Purpose |
|-------|-------|---------|
| MAX_OBJECT_SIZE | 100 MB | Prevent memory exhaustion |
| MAX_FILE_SIZE | 100 MB | Prevent large file attacks |
| MAX_TREE_ENTRIES | 10,000 | Limit tree complexity |
| MAX_TREE_DEPTH | 100 | Prevent stack overflow |
| MAX_PATH_LEN | 4096 | Path buffer limit |
| MAX_MESSAGE_SIZE | 10 KB | Commit message limit |

### Compiler Hardening

```makefile
-fstack-protector-strong    # Stack canaries
-fstack-clash-protection    # Stack clash mitigation
-D_FORTIFY_SOURCE=2         # Buffer overflow detection
-fPIE / -pie                # ASLR support
-Wl,-z,relro,-z,now        # GOT protection
-fcf-protection=full        # Control-flow integrity (x86_64)
```

## Future Development Roadmap

### Phase 1: Core Improvements
- [ ] Nested tree support in index (subdirectories)
- [ ] Pack file support for efficient storage
- [ ] Delta compression
- [ ] Loose object garbage collection

### Phase 2: Branch Operations
- [ ] git branch (create/list/delete)
- [ ] git checkout (switch branches)
- [ ] git merge (fast-forward only initially)
- [ ] git rebase (simple linear rebase)

### Phase 3: Remote Operations
- [ ] git remote (add/remove/list)
- [ ] git fetch (HTTP smart protocol)
- [ ] git push (HTTP smart protocol)
- [ ] git clone (shallow clone support)

### Phase 4: Advanced Features
- [ ] .gitignore support
- [ ] git diff (file comparison)
- [ ] git stash (temporary storage)
- [ ] git tag (annotated tags)

## Testing Strategy

### Unit Tests (tests/)
- TAP-compatible output format
- FIPS 180-2 test vectors for SHA-1
- Edge case coverage

### Integration Tests (make test)
- End-to-end workflow testing
- Repository initialization
- Object creation and retrieval

### Fuzz Testing (fuzz/)
- libFuzzer harnesses
- AddressSanitizer integration
- Continuous fuzzing recommended

### Static Analysis
- `make cppcheck` - cppcheck analysis
- `make scan-build` - clang static analyzer

## Contributing Guidelines

1. **Security First**: All code must be secure by default
2. **Error Handling**: Check all return values, handle all errors
3. **Memory Safety**: No leaks, no overflows, no use-after-free
4. **Testing**: Add tests for new functionality
5. **Documentation**: Update this file for architectural changes

## References

- [Git Internals](https://git-scm.com/book/en/v2/Git-Internals-Plumbing-and-Porcelain)
- [Git Index Format](https://git-scm.com/docs/index-format)
- [Git Pack Format](https://git-scm.com/docs/pack-format)
- [OpenSSF Compiler Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++)
- [CERT C Secure Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)
