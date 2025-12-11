/**
 * @file git_index.h
 * @brief Git index (staging area) interface
 *
 * The index is the staging area between the working directory and
 * the repository. It stores information about files that will be
 * included in the next commit.
 *
 * Index file format (.git/index):
 * - Header: signature ("DIRC"), version, entry count
 * - Entries: sorted list of file metadata + SHA-1
 * - Checksum: SHA-1 of the entire file content
 */

#ifndef GIT_INDEX_H
#define GIT_INDEX_H

#include "git_common.h"
#include <stdint.h>
#include <time.h>

// Index file constants
#define GIT_INDEX_SIGNATURE "DIRC"
#define GIT_INDEX_VERSION 2
#define GIT_INDEX_PATH ".git/index"

// Index entry flags
#define GIT_INDEX_ENTRY_NAMEMASK  0x0FFF
#define GIT_INDEX_ENTRY_STAGEMASK 0x3000
#define GIT_INDEX_ENTRY_STAGESHIFT 12

// File modes
#define GIT_FILEMODE_BLOB           0100644
#define GIT_FILEMODE_BLOB_EXECUTABLE 0100755
#define GIT_FILEMODE_LINK           0120000
#define GIT_FILEMODE_TREE           0040000
#define GIT_FILEMODE_COMMIT         0160000

/**
 * Index entry representing a staged file
 */
typedef struct {
    uint32_t ctime_sec;      // Creation time (seconds)
    uint32_t ctime_nsec;     // Creation time (nanoseconds)
    uint32_t mtime_sec;      // Modification time (seconds)
    uint32_t mtime_nsec;     // Modification time (nanoseconds)
    uint32_t dev;            // Device ID
    uint32_t ino;            // Inode number
    uint32_t mode;           // File mode
    uint32_t uid;            // User ID
    uint32_t gid;            // Group ID
    uint32_t file_size;      // File size
    unsigned char sha1[SHA1_DIGEST_SIZE];  // Object SHA-1
    uint16_t flags;          // Entry flags (12-bit name length + stage)
    char *path;              // File path (variable length)
} git_index_entry;

/**
 * Git index (staging area)
 */
typedef struct {
    uint32_t version;        // Index format version
    git_index_entry *entries;// Array of entries
    size_t entry_count;      // Number of entries
    size_t capacity;         // Allocated capacity
    int dirty;               // Whether index needs to be written
} git_index;

// Index lifecycle
git_index* git_index_new(void);
void git_index_free(git_index *index);

// Index I/O
int git_index_read(git_index *index, const char *path);
int git_index_write(git_index *index, const char *path);

// Entry management
int git_index_add(git_index *index, const char *path, const unsigned char sha1[SHA1_DIGEST_SIZE], uint32_t mode);
int git_index_add_from_workdir(git_index *index, const char *path);
int git_index_remove(git_index *index, const char *path);
git_index_entry* git_index_get(git_index *index, const char *path);
int git_index_clear(git_index *index);

// Index queries
size_t git_index_entrycount(const git_index *index);
const git_index_entry* git_index_get_byindex(const git_index *index, size_t n);
const git_index_entry* git_index_get_bypath(const git_index *index, const char *path);

// Tree operations
int git_index_write_tree(git_index *index, unsigned char sha1[SHA1_DIGEST_SIZE]);
int git_index_read_tree(git_index *index, const char *tree_sha1_hex);

#endif // GIT_INDEX_H
