/**
 * @file add.c
 * @brief Implementation of git add command
 *
 * TODO(Phase 1): .gitignore support
 *   - Check files against ignore patterns before adding
 *   - Support --force to add ignored files
 *
 * TODO(Phase 1): Intent-to-add (-N)
 *   - Add file to index with empty blob
 *   - Show as "new file" in status but not staged
 *
 * TODO(Phase 2): Interactive mode (-i/-p)
 *   - Patch selection (git add -p)
 *   - Interactive file selection
 *
 * TODO(Phase 2): Update mode (-u)
 *   - Only update already-tracked files
 *   - Don't add new files
 *
 * TODO(Phase 3): Refresh mode
 *   - git add --refresh (update stat info only)
 *   - Useful after touching files without changing content
 */

#include "git_common.h"
#include "git_index.h"
#include "git_validation.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Forward declaration for recursive add
static int add_path_recursive(git_index *index, const char *path);

static int add_single_file(git_index *index, const char *path) {
    int ret = git_index_add_from_workdir(index, path);
    if (ret == GIT_OK) {
        printf("add '%s'\n", path);
    }
    return ret;
}

static int add_directory(git_index *index, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", path);
        return GIT_ERROR;
    }

    struct dirent *entry;
    int ret = GIT_OK;

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip .git
        if (strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        // Build full path
        char full_path[MAX_PATH_LEN];
        int len = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (len < 0 || len >= (int)sizeof(full_path)) {
            fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
            continue;
        }

        ret = add_path_recursive(index, full_path);
        if (ret != GIT_OK) {
            break;
        }
    }

    closedir(dir);
    return ret;
}

static int add_path_recursive(git_index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "pathspec '%s' did not match any files\n", path);
        return GIT_ENOTFOUND;
    }

    if (S_ISDIR(st.st_mode)) {
        return add_directory(index, path);
    } else if (S_ISREG(st.st_mode)) {
        return add_single_file(index, path);
    } else {
        fprintf(stderr, "Skipping non-regular file: %s\n", path);
        return GIT_OK;
    }
}

int cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: add <file>...\n");
        fprintf(stderr, "       add .           Add all files\n");
        return GIT_EINVALID;
    }

    // Create/open index
    git_index *index = git_index_new();
    if (!index) {
        fprintf(stderr, "Failed to create index\n");
        return GIT_ERROR;
    }

    // Read existing index
    int ret = git_index_read(index, GIT_INDEX_PATH);
    if (ret != GIT_OK && ret != GIT_ENOTFOUND) {
        fprintf(stderr, "Failed to read index: %s\n", git_error_last());
        git_index_free(index);
        return ret;
    }

    // Add each specified path
    int any_error = 0;
    for (int i = 2; i < argc; i++) {
        const char *path = argv[i];

        // Handle "." specially - add all files
        if (strcmp(path, ".") == 0) {
            ret = add_directory(index, ".");
        } else {
            if (!git_validate_safe_path(path)) {
                fprintf(stderr, "Invalid path: %s\n", path);
                any_error = 1;
                continue;
            }
            ret = add_path_recursive(index, path);
        }

        if (ret != GIT_OK) {
            fprintf(stderr, "Failed to add '%s': %s\n", path, git_error_last());
            any_error = 1;
        }
    }

    // Write index if modified
    if (index->dirty) {
        ret = git_index_write(index, GIT_INDEX_PATH);
        if (ret != GIT_OK) {
            fprintf(stderr, "Failed to write index: %s\n", git_error_last());
            git_index_free(index);
            return ret;
        }
    }

    git_index_free(index);
    return any_error ? GIT_ERROR : GIT_OK;
}
