# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Support for cloning public GitHub repositories
  - Implement packfile processing and object extraction
  - Create proper Git directory structure with references
  - Handle commit history and branch references
  - Incorporate fallback mechanisms for robust operation
- `commit-tree` command with proper object format
  - Implement commit object creation and storage
  - Add parent commit support
  - Add timestamp and timezone handling
- `write-tree` command with recursive directory traversal
  - Implement proper Git tree object format and storage
  - Add support for both files and directories
  - Implement Git-compatible entry sorting
- `ls-tree` command with `--name-only` flag support
  - Implement tree object parsing and display
  - Add multi-platform decompression support for tree objects
- `hash-object` command with `-w` flag support
- `cat-file -p` command with multi-platform decompression
- Custom SHA-1 implementation for hash calculations
- Basic `.git` directory structure (`.git`, `.git/objects`, `.git/refs`)
- Initialize HEAD file pointing to refs/heads/main

### Changed
- Replace direct libcurl dependencies with system commands
- Remove OpenSSL dependency in favor of custom SHA-1

### Fixed
- Add robust error handling and validation
- Add error handling for directory creation and file operations
- Implement multiple fallback methods for object decompression