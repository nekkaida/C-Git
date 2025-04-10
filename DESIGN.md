# Mini Git Implementation Design

This document outlines the design decisions and architecture of our lightweight Git implementation.

## Core Design Philosophy

The implementation follows Git's fundamental content-addressable storage system, where all objects are identified by their SHA-1 hash. This design maintains Git's key properties:

1. **Content-addressable storage**: Objects are referenced by their content hash
2. **Immutable objects**: Once created, objects cannot be modified
3. **Directed acyclic graph**: Commits form a DAG with references to parent commits

## Object Model

The implementation uses Git's standard object types:

### Blob Objects
- Represent file contents
- Stored with header: `"blob <size>\0<content>"`
- Used by the `hash-object` command

### Tree Objects
- Represent directories and file hierarchies
- Contain entries with mode, name, and SHA-1 hash
- Format: `"tree <size>\0<entries>"` where entries are `"<mode> <name>\0<20-byte SHA>"`
- Used by `write-tree` and `ls-tree` commands

### Commit Objects
- Represent snapshots of the repository
- Contains tree hash, parent commit(s), author/committer information, and message
- Format: `"commit <size>\0tree <tree-sha>\nparent <parent-sha>\nauthor ...\ncommitter ...\n\n<message>"`
- Created with `commit-tree` command

## Storage Implementation

Objects are stored following Git's approach:

1. Object content is prefixed with a type and size header
2. The entire content is hashed with SHA-1
3. Objects are zlib-compressed 
4. Files are stored in `.git/objects/<first-2-chars>/<remaining-38-chars>`

For compression, the implementation takes a pragmatic approach by using system commands to leverage existing zlib libraries in Perl or Python, with multiple fallback mechanisms for compatibility.

## Command Implementation

Each Git command is implemented as a separate function:

- `cmd_init`: Creates basic Git directory structure
- `cmd_hash_object`: Computes SHA-1 hash of file content and optionally stores it
- `cmd_cat_file`: Retrieves and displays object content
- `cmd_ls_tree`: Lists contents of a tree object
- `cmd_write_tree`: Creates a tree object from the current directory
- `cmd_commit_tree`: Creates a commit object
- `cmd_clone`: Clones a GitHub repository

## Hash Function Implementation

The implementation includes a complete SHA-1 algorithm based on the standard specification. This provides:

1. Independence from external SHA-1 libraries
2. Consistent hashing across platforms
3. Full compatibility with Git's object addressing

## Network Operations

For network operations (cloning), the implementation:

1. Uses libcurl for HTTP requests when available
2. Falls back to system `curl` command when necessary
3. Implements GitHub-specific protocols for fetching repository data
4. Uses Git's packfile format for efficient object transfer

## Error Handling and Robustness

The implementation employs several strategies for robustness:

1. Multiple decompression methods (Perl, Python3, Python2) for maximum compatibility
2. Validation of object formats and hashes
3. Error reporting for common failure cases
4. Graceful fallbacks when primary methods fail

## Limitations and Design Tradeoffs

Some design decisions reflect tradeoffs between completeness, complexity, and practicality:

1. **Limited index support**: The implementation focuses on object storage rather than the working directory index
2. **Simplified remote operations**: Only basic GitHub cloning is supported
3. **External compression dependencies**: Using system commands for compression instead of direct zlib integration
4. **No reference management**: Limited support for branches and tags
5. **Simplified authentication**: No support for SSH or authenticated HTTPS

## Future Design Directions

The modular design allows for natural extension in several areas:

1. Adding index support for staging changes
2. Implementing reference management for branches and tags
3. Supporting more remote operations (push, fetch)
4. Improving packfile handling
5. Adding merge capabilities

## Performance Considerations

The implementation makes several performance tradeoffs:

1. Object compression is handled externally, which is slower but more portable
2. Tree sorting follows Git's behavior for compatibility
3. No object packing for local storage optimization
4. Minimal caching of objects or file system operations