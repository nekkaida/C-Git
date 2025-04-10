# Mini Git Implementation

This is a lightweight implementation of Git's core functionality written in C. It provides a subset of Git's features, focusing on the fundamental operations that make Git work.

## Features

- **Basic Git Operations**: Implements core Git commands like `init`, `hash-object`, `cat-file`, `ls-tree`, `write-tree`, and `commit-tree`
- **Object Storage**: Uses Git's object model (blobs, trees, commits)
- **SHA-1 Hashing**: Implements Git's content-addressable storage system
- **Cloning**: Basic support for cloning from GitHub repositories

## Commands

### Initialization
```
./main init
```
Initializes a new Git repository in the current directory.

### Object Manipulation
```
./main hash-object [-w] <file>
```
Computes the SHA-1 hash of a file and optionally writes it to the object store.

```
./main cat-file -p <object-hash>
```
Displays the contents of a Git object.

### Tree Operations
```
./main ls-tree [--name-only] <tree-hash>
```
Lists the contents of a tree object.

```
./main write-tree
```
Creates a tree object from the current directory and returns its hash.

### Commit Operations
```
./main commit-tree <tree-hash> -p <parent-commit> -m <message>
```
Creates a commit object from a tree and returns its hash.

### Repository Operations
```
./main clone <repository-url> <directory>
```
Clones a GitHub repository to the specified directory.

## Implementation Details

- Uses SHA-1 for content-addressable storage
- Implements zlib compression for object storage (using system calls to Perl/Python for compression)
- Storage format matches Git's internal format

## Dependencies

- Standard C libraries
- libcurl for HTTP operations
- zlib (accessed through Perl/Python for compatibility)
- Basic Unix utilities

## Building

Compile with:
```
make
```

Or manually:
```
gcc -o main main.c -lcurl -lz
```

## Limitations

- Subset of Git functionality
- Limited error handling
- No support for advanced features like branching, merging, etc.
- GitHub-specific implementation for remote operations

## License

MIT License