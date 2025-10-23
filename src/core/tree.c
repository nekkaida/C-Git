#include "git_tree.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

struct git_tree_builder {
    git_tree_entry *entries;
    size_t count;
    size_t capacity;
};

// Git-compatible tree sorting
// Directories are sorted as if they have trailing '/'
static int compare_tree_entries(const void *a, const void *b) {
    const git_tree_entry *ea = (const git_tree_entry *)a;
    const git_tree_entry *eb = (const git_tree_entry *)b;

    // Check if entries are directories
    int a_is_dir = (strcmp(ea->mode, GIT_MODE_DIR) == 0 || strcmp(ea->mode, "040000") == 0);
    int b_is_dir = (strcmp(eb->mode, GIT_MODE_DIR) == 0 || strcmp(eb->mode, "040000") == 0);

    // Create comparison strings (add '/' for directories)
    char name_a[258], name_b[258];
    int ret_a = snprintf(name_a, sizeof(name_a), "%s%s", ea->name, a_is_dir ? "/" : "");
    int ret_b = snprintf(name_b, sizeof(name_b), "%s%s", eb->name, b_is_dir ? "/" : "");

    // If truncated, handle gracefully (compare up to truncation point)
    if (ret_a < 0 || ret_a >= (int)sizeof(name_a)) {
        name_a[sizeof(name_a) - 1] = '\0';
    }
    if (ret_b < 0 || ret_b >= (int)sizeof(name_b)) {
        name_b[sizeof(name_b) - 1] = '\0';
    }

    return strcmp(name_a, name_b);
}

git_tree_builder* git_tree_builder_new(void) {
    git_tree_builder *builder = (git_tree_builder *)calloc(1, sizeof(git_tree_builder));
    if (!builder) {
        git_error_set(GIT_ENOMEM, "Failed to allocate tree builder");
        return NULL;
    }

    builder->capacity = 16;
    builder->entries = (git_tree_entry *)malloc(sizeof(git_tree_entry) * builder->capacity);
    if (!builder->entries) {
        free(builder);
        git_error_set(GIT_ENOMEM, "Failed to allocate entries");
        return NULL;
    }

    return builder;
}

void git_tree_builder_free(git_tree_builder *builder) {
    if (!builder) return;
    if (builder->entries) free(builder->entries);
    free(builder);
}

int git_tree_builder_add(git_tree_builder *builder, const char *mode,
                         const char *name, const unsigned char sha1[SHA1_DIGEST_SIZE]) {
    if (!builder || !mode || !name || !sha1) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_tree_builder_add");
        return GIT_EINVALID;
    }

    // Validate mode
    if (!git_validate_mode(mode)) {
        git_error_set(GIT_EINVALID, "Invalid mode");
        return GIT_EINVALID;
    }

    // Check if we need to resize - FIX REALLOC BUG!
    if (builder->count >= builder->capacity) {
        size_t new_capacity = builder->capacity * 2;
        git_tree_entry *new_entries = (git_tree_entry *)realloc(
            builder->entries,
            sizeof(git_tree_entry) * new_capacity
        );

        if (!new_entries) {
            // Keep old entries valid on failure
            git_error_set(GIT_ENOMEM, "Failed to resize entries array");
            return GIT_ENOMEM;
        }

        builder->entries = new_entries;
        builder->capacity = new_capacity;
    }

    // Add entry
    git_tree_entry *entry = &builder->entries[builder->count];

    if (git_safe_strncpy(entry->mode, mode, sizeof(entry->mode)) != GIT_OK) {
        git_error_set(GIT_EBUFSIZE, "Mode too long");
        return GIT_EBUFSIZE;
    }

    if (git_safe_strncpy(entry->name, name, sizeof(entry->name)) != GIT_OK) {
        git_error_set(GIT_EBUFSIZE, "Name too long");
        return GIT_EBUFSIZE;
    }

    memcpy(entry->sha1, sha1, SHA1_DIGEST_SIZE);
    builder->count++;

    return GIT_OK;
}

int git_tree_builder_write(git_tree_builder *builder, unsigned char sha1[SHA1_DIGEST_SIZE]) {
    if (!builder || !sha1) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_tree_builder_write");
        return GIT_EINVALID;
    }

    if (builder->count == 0) {
        git_error_set(GIT_EINVALID, "Cannot write empty tree");
        return GIT_EINVALID;
    }

    // Sort entries (Git-compatible sorting)
    qsort(builder->entries, builder->count, sizeof(git_tree_entry), compare_tree_entries);

    // Calculate total size with overflow checking
    size_t total_size = 0;
    for (size_t i = 0; i < builder->count; i++) {
        // "<mode> <name>\0<20-byte-sha>"
        size_t mode_len = strlen(builder->entries[i].mode);
        size_t name_len = strlen(builder->entries[i].name);
        size_t entry_size = mode_len + 1 + name_len + 1 + SHA1_DIGEST_SIZE;

        // Check for overflow in entry_size calculation
        if (entry_size < mode_len || entry_size < name_len) {
            git_error_set(GIT_EOVERFLOW, "Entry size overflow");
            return GIT_EOVERFLOW;
        }

        // Check for overflow in total_size accumulation
        if (total_size > SIZE_MAX - entry_size) {
            git_error_set(GIT_EOVERFLOW, "Tree size overflow");
            return GIT_EOVERFLOW;
        }

        total_size += entry_size;
    }

    // Sanity check: trees shouldn't be larger than MAX_FILE_SIZE
    if (total_size > MAX_FILE_SIZE) {
        git_error_set(GIT_EOVERFLOW, "Tree too large");
        return GIT_EOVERFLOW;
    }

    // Allocate buffer
    unsigned char *data = (unsigned char *)malloc(total_size);
    if (!data) {
        git_error_set(GIT_ENOMEM, "Failed to allocate tree data");
        return GIT_ENOMEM;
    }

    // Build tree data
    size_t offset = 0;
    for (size_t i = 0; i < builder->count; i++) {
        git_tree_entry *e = &builder->entries[i];

        // Write "<mode> <name>\0"
        size_t remaining = total_size - offset;
        int len = snprintf((char *)data + offset, remaining, "%s %s", e->mode, e->name);

        if (len < 0 || len >= (int)remaining) {
            free(data);
            git_error_set(GIT_EBUFSIZE, "Buffer overflow in tree data");
            return GIT_EBUFSIZE;
        }

        offset += len;
        data[offset++] = '\0';

        // Write 20-byte SHA-1
        memcpy(data + offset, e->sha1, SHA1_DIGEST_SIZE);
        offset += SHA1_DIGEST_SIZE;
    }

    // Create object
    git_object *obj = git_object_new(GIT_OBJ_TREE, total_size);
    if (!obj) {
        free(data);
        return GIT_ENOMEM;
    }

    memcpy(obj->data, data, total_size);
    free(data);

    // Write object
    int ret = git_object_write(obj);
    if (ret != GIT_OK) {
        git_object_free(obj);
        return ret;
    }

    // Copy SHA-1
    memcpy(sha1, obj->sha1, SHA1_DIGEST_SIZE);
    git_object_free(obj);

    return GIT_OK;
}

// Recursive function to build tree from directory
static int write_tree_recursive(const char *path, unsigned char sha1[SHA1_DIGEST_SIZE],
                                 int *depth) {
    if (!path || !sha1 || !depth) {
        git_error_set(GIT_EINVALID, "Invalid parameters");
        return GIT_EINVALID;
    }

    // Check recursion depth
    if (*depth >= MAX_TREE_DEPTH) {
        git_error_set(GIT_ERROR, "Maximum tree depth exceeded");
        return GIT_ERROR;
    }
    (*depth)++;

    DIR *dir = opendir(path);
    if (!dir) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to open directory %s: %s",
                 path, strerror(errno));
        git_error_set(GIT_ERROR, msg);
        (*depth)--;
        return GIT_ERROR;
    }

    git_tree_builder *builder = git_tree_builder_new();
    if (!builder) {
        closedir(dir);
        (*depth)--;
        return GIT_ENOMEM;
    }

    struct dirent *entry;
    int ret = GIT_OK;

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip .git
        if (strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        // Build full path
        char full_path[MAX_PATH_LEN];
        ret = git_safe_path_join(full_path, sizeof(full_path), path, entry->d_name);
        if (ret != GIT_OK) {
            fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) != 0) {
            fprintf(stderr, "Failed to stat %s: %s\n", full_path, strerror(errno));
            continue;
        }

        unsigned char entry_sha1[SHA1_DIGEST_SIZE];

        if (S_ISDIR(st.st_mode)) {
            // Recursively write tree
            ret = write_tree_recursive(full_path, entry_sha1, depth);
            if (ret != GIT_OK) {
                fprintf(stderr, "Failed to write tree for %s\n", full_path);
                break;
            }

            ret = git_tree_builder_add(builder, GIT_MODE_DIR, entry->d_name, entry_sha1);
        } else if (S_ISREG(st.st_mode)) {
            // Hash file
            FILE *file = fopen(full_path, "rb");
            if (!file) {
                fprintf(stderr, "Failed to open %s: %s\n", full_path, strerror(errno));
                continue;
            }

            if (fseek(file, 0, SEEK_END) != 0) {
                fclose(file);
                fprintf(stderr, "Failed to seek in file %s\n", full_path);
                continue;
            }

            long size = ftell(file);

            if (fseek(file, 0, SEEK_SET) != 0) {
                fclose(file);
                fprintf(stderr, "Failed to seek to beginning of %s\n", full_path);
                continue;
            }

            if (size < 0 || size > MAX_FILE_SIZE) {
                fclose(file);
                fprintf(stderr, "File %s too large or size error\n", full_path);
                continue;
            }

            void *content = malloc(size);
            if (!content) {
                fclose(file);
                ret = GIT_ENOMEM;
                break;
            }

            size_t bytes_read = fread(content, 1, size, file);
            fclose(file);

            if (bytes_read != (size_t)size) {
                free(content);
                fprintf(stderr, "Failed to read complete file %s: expected %ld bytes, got %zu bytes\n",
                        full_path, size, bytes_read);
                ret = GIT_ERROR;
                break;
            }

            // Create and write blob
            git_object *blob = git_object_new(GIT_OBJ_BLOB, size);
            if (!blob) {
                free(content);
                ret = GIT_ENOMEM;
                break;
            }

            memcpy(blob->data, content, size);
            free(content);

            ret = git_object_write(blob);
            if (ret == GIT_OK) {
                memcpy(entry_sha1, blob->sha1, SHA1_DIGEST_SIZE);
            }
            git_object_free(blob);

            if (ret != GIT_OK) {
                fprintf(stderr, "Failed to write blob for %s\n", full_path);
                break;
            }

            // Determine mode (check if executable)
            const char *mode = (st.st_mode & S_IXUSR) ? GIT_MODE_EXEC : GIT_MODE_FILE;
            ret = git_tree_builder_add(builder, mode, entry->d_name, entry_sha1);
        } else {
            // Skip other file types
            continue;
        }

        if (ret != GIT_OK) {
            break;
        }
    }

    closedir(dir);
    (*depth)--;

    if (ret == GIT_OK && builder->count > 0) {
        ret = git_tree_builder_write(builder, sha1);
    } else if (builder->count == 0) {
        git_error_set(GIT_ERROR, "Empty directory");
        ret = GIT_ERROR;
    }

    git_tree_builder_free(builder);
    return ret;
}

int git_tree_write_from_directory(const char *path, unsigned char sha1[SHA1_DIGEST_SIZE]) {
    int depth = 0;
    return write_tree_recursive(path, sha1, &depth);
}
