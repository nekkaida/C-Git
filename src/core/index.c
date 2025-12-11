/**
 * @file index.c
 * @brief Git index (staging area) implementation
 *
 * TODO(Phase 1): Nested tree support
 *   - Build hierarchical tree structure from flat index paths
 *   - Handle paths like "dir/subdir/file.c" properly
 *   - Create intermediate tree objects for directories
 *
 * TODO(Phase 1): Index v3/v4 support
 *   - Add extended flags support (v3)
 *   - Add path prefix compression (v4)
 *
 * TODO(Phase 2): Conflict markers
 *   - Stage 1/2/3 support for merge conflicts
 *   - Conflict resolution helpers
 *
 * TODO(Phase 3): Index extensions
 *   - TREE extension (cached tree)
 *   - REUC extension (resolve undo)
 *   - UNTR extension (untracked cache)
 */

#include "git_index.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>  // For ntohl/htonl
#else
#include <arpa/inet.h>  // For ntohl/htonl
#endif

// Initial capacity for entries
#define INITIAL_CAPACITY 64

// Helper to read 32-bit big-endian integer
static uint32_t read_be32(const unsigned char *data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8)  |
           ((uint32_t)data[3]);
}

// Helper to write 32-bit big-endian integer
static void write_be32(unsigned char *data, uint32_t value) {
    data[0] = (unsigned char)(value >> 24);
    data[1] = (unsigned char)(value >> 16);
    data[2] = (unsigned char)(value >> 8);
    data[3] = (unsigned char)(value);
}

// Helper to read 16-bit big-endian integer
static uint16_t read_be16(const unsigned char *data) {
    return (uint16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
}

// Helper to write 16-bit big-endian integer
static void write_be16(unsigned char *data, uint16_t value) {
    data[0] = (unsigned char)(value >> 8);
    data[1] = (unsigned char)(value);
}

// Compare function for sorting entries by path
static int compare_entries(const void *a, const void *b) {
    const git_index_entry *ea = (const git_index_entry *)a;
    const git_index_entry *eb = (const git_index_entry *)b;
    return strcmp(ea->path, eb->path);
}

git_index* git_index_new(void) {
    git_index *index = (git_index *)calloc(1, sizeof(git_index));
    if (!index) {
        git_error_set(GIT_ENOMEM, "Failed to allocate index");
        return NULL;
    }

    index->version = GIT_INDEX_VERSION;
    index->capacity = INITIAL_CAPACITY;
    index->entries = (git_index_entry *)calloc(index->capacity, sizeof(git_index_entry));
    if (!index->entries) {
        free(index);
        git_error_set(GIT_ENOMEM, "Failed to allocate index entries");
        return NULL;
    }

    return index;
}

void git_index_free(git_index *index) {
    if (!index) return;

    for (size_t i = 0; i < index->entry_count; i++) {
        free(index->entries[i].path);
    }
    free(index->entries);
    free(index);
}

int git_index_read(git_index *index, const char *path) {
    if (!index || !path) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        // No index file is OK - just start empty
        if (errno == ENOENT) {
            return GIT_OK;
        }
        git_error_set(GIT_ERROR, "Failed to open index file");
        return GIT_ERROR;
    }

    // Read header (12 bytes)
    unsigned char header[12];
    if (fread(header, 1, 12, file) != 12) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to read index header");
        return GIT_ERROR;
    }

    // Verify signature
    if (memcmp(header, GIT_INDEX_SIGNATURE, 4) != 0) {
        fclose(file);
        git_error_set(GIT_ERROR, "Invalid index signature");
        return GIT_ERROR;
    }

    // Read version
    uint32_t version = read_be32(header + 4);
    if (version < 2 || version > 4) {
        fclose(file);
        git_error_set(GIT_ERROR, "Unsupported index version");
        return GIT_ERROR;
    }
    index->version = version;

    // Read entry count
    uint32_t entry_count = read_be32(header + 8);
    if (entry_count > MAX_TREE_ENTRIES) {
        fclose(file);
        git_error_set(GIT_EOVERFLOW, "Too many index entries");
        return GIT_EOVERFLOW;
    }

    // Clear existing entries
    git_index_clear(index);

    // Read each entry
    for (uint32_t i = 0; i < entry_count; i++) {
        // Read fixed-size entry data (62 bytes)
        unsigned char entry_data[62];
        if (fread(entry_data, 1, 62, file) != 62) {
            fclose(file);
            git_error_set(GIT_ERROR, "Failed to read index entry");
            return GIT_ERROR;
        }

        git_index_entry entry = {0};
        entry.ctime_sec = read_be32(entry_data);
        entry.ctime_nsec = read_be32(entry_data + 4);
        entry.mtime_sec = read_be32(entry_data + 8);
        entry.mtime_nsec = read_be32(entry_data + 12);
        entry.dev = read_be32(entry_data + 16);
        entry.ino = read_be32(entry_data + 20);
        entry.mode = read_be32(entry_data + 24);
        entry.uid = read_be32(entry_data + 28);
        entry.gid = read_be32(entry_data + 32);
        entry.file_size = read_be32(entry_data + 36);
        memcpy(entry.sha1, entry_data + 40, SHA1_DIGEST_SIZE);
        entry.flags = read_be16(entry_data + 60);

        // Get path length from flags
        size_t path_len = entry.flags & GIT_INDEX_ENTRY_NAMEMASK;
        if (path_len == GIT_INDEX_ENTRY_NAMEMASK) {
            // Extended path - read until null byte
            // For simplicity, limit to MAX_PATH_LEN
            path_len = 0;
        }

        // Read path
        char path_buf[MAX_PATH_LEN];
        size_t bytes_read = 0;
        int c;
        while ((c = fgetc(file)) != EOF && c != '\0' && bytes_read < MAX_PATH_LEN - 1) {
            path_buf[bytes_read++] = (char)c;
        }
        path_buf[bytes_read] = '\0';

        entry.path = strdup(path_buf);
        if (!entry.path) {
            fclose(file);
            git_error_set(GIT_ENOMEM, "Failed to allocate path");
            return GIT_ENOMEM;
        }

        // Skip padding to 8-byte boundary
        size_t entry_size = 62 + bytes_read + 1;  // Fixed + path + null
        size_t padding = (8 - (entry_size % 8)) % 8;
        if (padding > 0) {
            fseek(file, (long)padding, SEEK_CUR);
        }

        // Add entry to index
        if (index->entry_count >= index->capacity) {
            size_t new_capacity = index->capacity * 2;
            git_index_entry *new_entries = (git_index_entry *)realloc(
                index->entries, sizeof(git_index_entry) * new_capacity);
            if (!new_entries) {
                free(entry.path);
                fclose(file);
                git_error_set(GIT_ENOMEM, "Failed to grow index");
                return GIT_ENOMEM;
            }
            index->entries = new_entries;
            index->capacity = new_capacity;
        }

        index->entries[index->entry_count++] = entry;
    }

    fclose(file);
    return GIT_OK;
}

int git_index_write(git_index *index, const char *path) {
    if (!index || !path) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    FILE *file = fopen(path, "wb");
    if (!file) {
        git_error_set(GIT_ERROR, "Failed to create index file");
        return GIT_ERROR;
    }

    // Sort entries by path
    if (index->entry_count > 1) {
        qsort(index->entries, index->entry_count,
              sizeof(git_index_entry), compare_entries);
    }

    // Initialize SHA-1 for checksum
    git_sha1_ctx checksum_ctx;
    git_sha1_init(&checksum_ctx);

    // Write header
    unsigned char header[12];
    memcpy(header, GIT_INDEX_SIGNATURE, 4);
    write_be32(header + 4, index->version);
    write_be32(header + 8, (uint32_t)index->entry_count);

    if (fwrite(header, 1, 12, file) != 12) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to write index header");
        return GIT_ERROR;
    }
    git_sha1_update(&checksum_ctx, header, 12);

    // Write entries
    for (size_t i = 0; i < index->entry_count; i++) {
        git_index_entry *entry = &index->entries[i];

        // Fixed-size entry data
        unsigned char entry_data[62];
        write_be32(entry_data, entry->ctime_sec);
        write_be32(entry_data + 4, entry->ctime_nsec);
        write_be32(entry_data + 8, entry->mtime_sec);
        write_be32(entry_data + 12, entry->mtime_nsec);
        write_be32(entry_data + 16, entry->dev);
        write_be32(entry_data + 20, entry->ino);
        write_be32(entry_data + 24, entry->mode);
        write_be32(entry_data + 28, entry->uid);
        write_be32(entry_data + 32, entry->gid);
        write_be32(entry_data + 36, entry->file_size);
        memcpy(entry_data + 40, entry->sha1, SHA1_DIGEST_SIZE);

        // Set flags (path length in lower 12 bits)
        size_t path_len = strlen(entry->path);
        uint16_t flags = (path_len < GIT_INDEX_ENTRY_NAMEMASK)
                         ? (uint16_t)path_len
                         : GIT_INDEX_ENTRY_NAMEMASK;
        write_be16(entry_data + 60, flags);

        if (fwrite(entry_data, 1, 62, file) != 62) {
            fclose(file);
            git_error_set(GIT_ERROR, "Failed to write index entry");
            return GIT_ERROR;
        }
        git_sha1_update(&checksum_ctx, entry_data, 62);

        // Write path with null terminator
        if (fwrite(entry->path, 1, path_len + 1, file) != path_len + 1) {
            fclose(file);
            git_error_set(GIT_ERROR, "Failed to write index entry path");
            return GIT_ERROR;
        }
        git_sha1_update(&checksum_ctx, entry->path, path_len + 1);

        // Padding to 8-byte boundary
        size_t entry_size = 62 + path_len + 1;
        size_t padding = (8 - (entry_size % 8)) % 8;
        if (padding > 0) {
            unsigned char zeros[8] = {0};
            if (fwrite(zeros, 1, padding, file) != padding) {
                fclose(file);
                git_error_set(GIT_ERROR, "Failed to write index padding");
                return GIT_ERROR;
            }
            git_sha1_update(&checksum_ctx, zeros, padding);
        }
    }

    // Write checksum
    unsigned char checksum[SHA1_DIGEST_SIZE];
    git_sha1_final(checksum, &checksum_ctx);
    if (fwrite(checksum, 1, SHA1_DIGEST_SIZE, file) != SHA1_DIGEST_SIZE) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to write index checksum");
        return GIT_ERROR;
    }

    fclose(file);
    index->dirty = 0;
    return GIT_OK;
}

int git_index_add(git_index *index, const char *path,
                  const unsigned char sha1[SHA1_DIGEST_SIZE], uint32_t mode) {
    if (!index || !path || !sha1) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    if (!git_validate_safe_path(path)) {
        git_error_set(GIT_EINVALID, "Invalid path");
        return GIT_EINVALID;
    }

    // Check if entry already exists
    for (size_t i = 0; i < index->entry_count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            // Update existing entry
            memcpy(index->entries[i].sha1, sha1, SHA1_DIGEST_SIZE);
            index->entries[i].mode = mode;
            index->dirty = 1;
            return GIT_OK;
        }
    }

    // Check capacity
    if (index->entry_count >= index->capacity) {
        size_t new_capacity = index->capacity * 2;
        if (new_capacity > MAX_TREE_ENTRIES) {
            new_capacity = MAX_TREE_ENTRIES;
        }
        if (index->entry_count >= new_capacity) {
            git_error_set(GIT_EOVERFLOW, "Index full");
            return GIT_EOVERFLOW;
        }

        git_index_entry *new_entries = (git_index_entry *)realloc(
            index->entries, sizeof(git_index_entry) * new_capacity);
        if (!new_entries) {
            git_error_set(GIT_ENOMEM, "Failed to grow index");
            return GIT_ENOMEM;
        }
        index->entries = new_entries;
        index->capacity = new_capacity;
    }

    // Add new entry
    git_index_entry *entry = &index->entries[index->entry_count];
    memset(entry, 0, sizeof(git_index_entry));

    entry->path = strdup(path);
    if (!entry->path) {
        git_error_set(GIT_ENOMEM, "Failed to allocate path");
        return GIT_ENOMEM;
    }

    memcpy(entry->sha1, sha1, SHA1_DIGEST_SIZE);
    entry->mode = mode;
    entry->flags = (uint16_t)(strlen(path) < GIT_INDEX_ENTRY_NAMEMASK
                              ? strlen(path)
                              : GIT_INDEX_ENTRY_NAMEMASK);

    index->entry_count++;
    index->dirty = 1;
    return GIT_OK;
}

int git_index_add_from_workdir(git_index *index, const char *path) {
    if (!index || !path) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    if (!git_validate_safe_path(path)) {
        git_error_set(GIT_EINVALID, "Invalid path");
        return GIT_EINVALID;
    }

    // Stat the file
    struct stat st;
    if (stat(path, &st) != 0) {
        git_error_set(GIT_ENOTFOUND, "File not found");
        return GIT_ENOTFOUND;
    }

    if (!S_ISREG(st.st_mode)) {
        git_error_set(GIT_EINVALID, "Not a regular file");
        return GIT_EINVALID;
    }

    // Read file content
    FILE *file = fopen(path, "rb");
    if (!file) {
        git_error_set(GIT_ERROR, "Failed to open file");
        return GIT_ERROR;
    }

    size_t file_size = (size_t)st.st_size;
    if (file_size > MAX_FILE_SIZE) {
        fclose(file);
        git_error_set(GIT_EOVERFLOW, "File too large");
        return GIT_EOVERFLOW;
    }

    void *content = malloc(file_size);
    if (!content && file_size > 0) {
        fclose(file);
        git_error_set(GIT_ENOMEM, "Failed to allocate content buffer");
        return GIT_ENOMEM;
    }

    if (file_size > 0 && fread(content, 1, file_size, file) != file_size) {
        free(content);
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to read file");
        return GIT_ERROR;
    }
    fclose(file);

    // Create blob object
    git_object *blob = git_object_new(GIT_OBJ_BLOB, file_size);
    if (!blob) {
        free(content);
        return GIT_ENOMEM;
    }

    if (file_size > 0) {
        memcpy(blob->data, content, file_size);
    }
    free(content);

    // Write blob to object store
    int ret = git_object_write(blob);
    if (ret != GIT_OK) {
        git_object_free(blob);
        return ret;
    }

    // Determine mode
    uint32_t mode = (st.st_mode & S_IXUSR) ? GIT_FILEMODE_BLOB_EXECUTABLE
                                            : GIT_FILEMODE_BLOB;

    // Add to index
    ret = git_index_add(index, path, blob->sha1, mode);

    // Update stat info
    if (ret == GIT_OK) {
        git_index_entry *entry = git_index_get(index, path);
        if (entry) {
            entry->ctime_sec = (uint32_t)st.st_ctime;
            entry->mtime_sec = (uint32_t)st.st_mtime;
            entry->dev = (uint32_t)st.st_dev;
            entry->ino = (uint32_t)st.st_ino;
            entry->uid = (uint32_t)st.st_uid;
            entry->gid = (uint32_t)st.st_gid;
            entry->file_size = (uint32_t)st.st_size;
        }
    }

    git_object_free(blob);
    return ret;
}

int git_index_remove(git_index *index, const char *path) {
    if (!index || !path) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    for (size_t i = 0; i < index->entry_count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            free(index->entries[i].path);

            // Shift remaining entries
            for (size_t j = i; j < index->entry_count - 1; j++) {
                index->entries[j] = index->entries[j + 1];
            }

            index->entry_count--;
            index->dirty = 1;
            return GIT_OK;
        }
    }

    git_error_set(GIT_ENOTFOUND, "Entry not found");
    return GIT_ENOTFOUND;
}

git_index_entry* git_index_get(git_index *index, const char *path) {
    if (!index || !path) return NULL;

    for (size_t i = 0; i < index->entry_count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            return &index->entries[i];
        }
    }

    return NULL;
}

int git_index_clear(git_index *index) {
    if (!index) {
        git_error_set(GIT_EINVALID, "Invalid index");
        return GIT_EINVALID;
    }

    for (size_t i = 0; i < index->entry_count; i++) {
        free(index->entries[i].path);
    }

    index->entry_count = 0;
    index->dirty = 1;
    return GIT_OK;
}

size_t git_index_entrycount(const git_index *index) {
    return index ? index->entry_count : 0;
}

const git_index_entry* git_index_get_byindex(const git_index *index, size_t n) {
    if (!index || n >= index->entry_count) return NULL;
    return &index->entries[n];
}

const git_index_entry* git_index_get_bypath(const git_index *index, const char *path) {
    if (!index || !path) return NULL;

    for (size_t i = 0; i < index->entry_count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            return &index->entries[i];
        }
    }

    return NULL;
}

int git_index_write_tree(git_index *index, unsigned char sha1[SHA1_DIGEST_SIZE]) {
    if (!index || !sha1) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    if (index->entry_count == 0) {
        git_error_set(GIT_ERROR, "Cannot write tree from empty index");
        return GIT_ERROR;
    }

    // Sort entries
    if (index->entry_count > 1) {
        qsort(index->entries, index->entry_count,
              sizeof(git_index_entry), compare_entries);
    }

    // TODO(Phase 1): Build nested tree structure for paths with '/'
    // Current limitation: Creates flat tree, doesn't handle subdirectories
    // Implementation steps:
    //   1. Group entries by first path component
    //   2. For directories, recursively create subtrees
    //   3. Add subtree SHA-1 with mode 040000
    //   4. Add files with mode 100644/100755

    // Calculate tree content size
    size_t content_size = 0;
    for (size_t i = 0; i < index->entry_count; i++) {
        // "<mode> <name>\0<20-byte-sha>"
        // mode is in octal string form (6 chars max)
        content_size += 7 + strlen(index->entries[i].path) + 1 + SHA1_DIGEST_SIZE;
    }

    unsigned char *tree_data = (unsigned char *)malloc(content_size);
    if (!tree_data) {
        git_error_set(GIT_ENOMEM, "Failed to allocate tree data");
        return GIT_ENOMEM;
    }

    // Build tree content
    size_t offset = 0;
    for (size_t i = 0; i < index->entry_count; i++) {
        git_index_entry *entry = &index->entries[i];

        // Convert mode to string (without leading zeros for files)
        char mode_str[8];
        if (entry->mode == GIT_FILEMODE_TREE) {
            snprintf(mode_str, sizeof(mode_str), "40000");
        } else if (entry->mode == GIT_FILEMODE_BLOB_EXECUTABLE) {
            snprintf(mode_str, sizeof(mode_str), "100755");
        } else {
            snprintf(mode_str, sizeof(mode_str), "100644");
        }

        // Write "<mode> <name>\0"
        int len = snprintf((char *)tree_data + offset, content_size - offset,
                          "%s %s", mode_str, entry->path);
        offset += (size_t)len;
        tree_data[offset++] = '\0';

        // Write SHA-1
        memcpy(tree_data + offset, entry->sha1, SHA1_DIGEST_SIZE);
        offset += SHA1_DIGEST_SIZE;
    }

    // Create tree object
    git_object *tree = git_object_new(GIT_OBJ_TREE, offset);
    if (!tree) {
        free(tree_data);
        return GIT_ENOMEM;
    }

    memcpy(tree->data, tree_data, offset);
    free(tree_data);

    // Write tree
    int ret = git_object_write(tree);
    if (ret == GIT_OK) {
        memcpy(sha1, tree->sha1, SHA1_DIGEST_SIZE);
    }

    git_object_free(tree);
    return ret;
}

int git_index_read_tree(git_index *index, const char *tree_sha1_hex) {
    if (!index || !tree_sha1_hex) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    if (!git_validate_sha1_hex(tree_sha1_hex)) {
        git_error_set(GIT_EINVALID, "Invalid SHA-1 hash");
        return GIT_EINVALID;
    }

    // Read tree object
    git_object *tree = NULL;
    int ret = git_object_read(tree_sha1_hex, &tree);
    if (ret != GIT_OK) {
        return ret;
    }

    if (tree->type != GIT_OBJ_TREE) {
        git_object_free(tree);
        git_error_set(GIT_EINVALID, "Object is not a tree");
        return GIT_EINVALID;
    }

    // Clear existing index
    git_index_clear(index);

    // Parse tree entries
    const unsigned char *data = (const unsigned char *)tree->data;
    const unsigned char *end = data + tree->size;

    while (data < end) {
        // Parse mode (until space)
        const unsigned char *space = memchr(data, ' ', (size_t)(end - data));
        if (!space) break;

        char mode_str[16];
        size_t mode_len = (size_t)(space - data);
        if (mode_len >= sizeof(mode_str)) {
            git_object_free(tree);
            git_error_set(GIT_ERROR, "Invalid tree entry mode");
            return GIT_ERROR;
        }
        memcpy(mode_str, data, mode_len);
        mode_str[mode_len] = '\0';

        uint32_t mode = (uint32_t)strtoul(mode_str, NULL, 8);
        data = space + 1;

        // Parse name (until null)
        const unsigned char *null_byte = memchr(data, '\0', (size_t)(end - data));
        if (!null_byte) break;

        size_t name_len = (size_t)(null_byte - data);
        if (name_len >= MAX_PATH_LEN) {
            git_object_free(tree);
            git_error_set(GIT_ERROR, "Path too long in tree");
            return GIT_ERROR;
        }

        char path[MAX_PATH_LEN];
        memcpy(path, data, name_len);
        path[name_len] = '\0';
        data = null_byte + 1;

        // Read SHA-1 (20 bytes)
        if (data + SHA1_DIGEST_SIZE > end) break;

        // Add entry to index (skip directories for flat index)
        if (mode != GIT_FILEMODE_TREE) {
            ret = git_index_add(index, path, data, mode);
            if (ret != GIT_OK) {
                git_object_free(tree);
                return ret;
            }
        }

        data += SHA1_DIGEST_SIZE;
    }

    git_object_free(tree);
    return GIT_OK;
}
