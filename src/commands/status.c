/**
 * @file status.c
 * @brief Implementation of git status command
 *
 * TODO(Phase 1): Compare against HEAD tree
 *   - Read HEAD commit's tree
 *   - Diff index against HEAD to show "staged" vs "to be committed"
 *   - Currently shows all index entries as "staged"
 *
 * TODO(Phase 1): .gitignore support
 *   - Parse .gitignore files (root and nested)
 *   - Filter untracked files based on patterns
 *   - Support negation patterns (!)
 *
 * TODO(Phase 2): Rename detection
 *   - Detect moved/renamed files
 *   - Show as "renamed: old -> new"
 *
 * TODO(Phase 2): Submodule support
 *   - Detect submodule changes
 *   - Show submodule status summary
 *
 * TODO(Phase 3): Performance
 *   - Use index stat cache for faster modified detection
 *   - Parallel directory scanning
 */

#include "git_common.h"
#include "git_index.h"
#include "git_sha1.h"
#include "git_object.h"
#include "git_validation.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Track files for status reporting
typedef struct {
    char **staged;
    size_t staged_count;
    char **modified;
    size_t modified_count;
    char **untracked;
    size_t untracked_count;
    size_t capacity;
} status_info;

static status_info* status_info_new(void) {
    status_info *info = (status_info *)calloc(1, sizeof(status_info));
    if (!info) return NULL;

    info->capacity = 1024;  // Support up to 1024 files per category
    info->staged = (char **)calloc(info->capacity, sizeof(char *));
    info->modified = (char **)calloc(info->capacity, sizeof(char *));
    info->untracked = (char **)calloc(info->capacity, sizeof(char *));

    if (!info->staged || !info->modified || !info->untracked) {
        free(info->staged);
        free(info->modified);
        free(info->untracked);
        free(info);
        return NULL;
    }

    return info;
}

static void status_info_free(status_info *info) {
    if (!info) return;

    for (size_t i = 0; i < info->staged_count; i++) free(info->staged[i]);
    for (size_t i = 0; i < info->modified_count; i++) free(info->modified[i]);
    for (size_t i = 0; i < info->untracked_count; i++) free(info->untracked[i]);

    free(info->staged);
    free(info->modified);
    free(info->untracked);
    free(info);
}

static int add_to_list(char ***list, size_t *count, size_t capacity, const char *path) {
    if (*count >= capacity) return -1;
    (*list)[*count] = strdup(path);
    if (!(*list)[*count]) return -1;
    (*count)++;
    return 0;
}

static int is_file_tracked(const git_index *index, const char *path) {
    return git_index_get_bypath(index, path) != NULL;
}

static int has_file_changed(const git_index *index, const char *path) {
    const git_index_entry *entry = git_index_get_bypath(index, path);
    if (!entry) return 0;

    struct stat st;
    if (stat(path, &st) != 0) {
        return 1;  // File deleted
    }

    // Quick check: compare mtime and size
    if (entry->mtime_sec != (uint32_t)st.st_mtime ||
        entry->file_size != (uint32_t)st.st_size) {
        return 1;
    }

    // TODO(Phase 1): Content-based change detection
    // For thorough check when stat matches but file may have changed:
    //   1. Read file content
    //   2. Compute SHA-1
    //   3. Compare with entry->sha1
    // This handles cases where file was modified and restored within same second
    return 0;
}

static void scan_directory(const git_index *index, const char *path,
                           status_info *info, int is_root) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip .git directory
        if (strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        const char *relative_path;

        if (is_root && strcmp(path, ".") == 0) {
            snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            relative_path = full_path;
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            relative_path = full_path;
        }

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_directory(index, full_path, info, 0);
        } else if (S_ISREG(st.st_mode)) {
            if (is_file_tracked(index, relative_path)) {
                if (has_file_changed(index, relative_path)) {
                    add_to_list(&info->modified, &info->modified_count,
                               info->capacity, relative_path);
                }
            } else {
                add_to_list(&info->untracked, &info->untracked_count,
                           info->capacity, relative_path);
            }
        }
    }

    closedir(dir);
}

static void read_head_commit(char *commit_sha, size_t size) {
    commit_sha[0] = '\0';

    FILE *head = fopen(".git/HEAD", "r");
    if (!head) return;

    char line[256];
    if (fgets(line, sizeof(line), head)) {
        if (strncmp(line, "ref: ", 5) == 0) {
            // Symbolic ref - read the ref file
            char ref_path[MAX_PATH_LEN];
            char *newline = strchr(line + 5, '\n');
            if (newline) *newline = '\0';

            snprintf(ref_path, sizeof(ref_path), ".git/%s", line + 5);

            FILE *ref = fopen(ref_path, "r");
            if (ref) {
                if (fgets(commit_sha, (int)size, ref)) {
                    char *nl = strchr(commit_sha, '\n');
                    if (nl) *nl = '\0';
                }
                fclose(ref);
            }
        } else {
            // Direct SHA
            strncpy(commit_sha, line, size - 1);
            commit_sha[size - 1] = '\0';
            char *nl = strchr(commit_sha, '\n');
            if (nl) *nl = '\0';
        }
    }

    fclose(head);
}

static const char* get_branch_name(void) {
    static char branch[256] = "main";

    FILE *head = fopen(".git/HEAD", "r");
    if (!head) return branch;

    char line[256];
    if (fgets(line, sizeof(line), head)) {
        if (strncmp(line, "ref: refs/heads/", 16) == 0) {
            char *newline = strchr(line + 16, '\n');
            if (newline) *newline = '\0';
            strncpy(branch, line + 16, sizeof(branch) - 1);
            branch[sizeof(branch) - 1] = '\0';
        }
    }

    fclose(head);
    return branch;
}

int cmd_status(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Check if we're in a git repository
    struct stat st;
    if (stat(".git", &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "fatal: not a git repository\n");
        return GIT_ERROR;
    }

    // Read index
    git_index *index = git_index_new();
    if (!index) {
        fprintf(stderr, "Failed to create index\n");
        return GIT_ERROR;
    }

    int ret = git_index_read(index, GIT_INDEX_PATH);
    if (ret != GIT_OK && ret != GIT_ENOTFOUND) {
        fprintf(stderr, "Failed to read index: %s\n", git_error_last());
        git_index_free(index);
        return ret;
    }

    // Get branch name
    const char *branch = get_branch_name();
    printf("On branch %s\n\n", branch);

    // Check if there are any commits
    char head_sha[SHA1_HEX_SIZE + 1];
    read_head_commit(head_sha, sizeof(head_sha));

    if (head_sha[0] == '\0') {
        printf("No commits yet\n\n");
    }

    // Collect status info
    status_info *info = status_info_new();
    if (!info) {
        git_index_free(index);
        return GIT_ENOMEM;
    }

    // Find staged files (entries in index)
    for (size_t i = 0; i < git_index_entrycount(index); i++) {
        const git_index_entry *entry = git_index_get_byindex(index, i);
        if (entry) {
            add_to_list(&info->staged, &info->staged_count,
                       info->capacity, entry->path);
        }
    }

    // Scan working directory
    scan_directory(index, ".", info, 1);

    // Print staged changes
    if (info->staged_count > 0) {
        printf("Changes to be committed:\n");
        printf("  (use \"git restore --staged <file>...\" to unstage)\n");
        for (size_t i = 0; i < info->staged_count; i++) {
            printf("\tnew file:   %s\n", info->staged[i]);
        }
        printf("\n");
    }

    // Print modified files
    if (info->modified_count > 0) {
        printf("Changes not staged for commit:\n");
        printf("  (use \"git add <file>...\" to update what will be committed)\n");
        for (size_t i = 0; i < info->modified_count; i++) {
            printf("\tmodified:   %s\n", info->modified[i]);
        }
        printf("\n");
    }

    // Print untracked files
    if (info->untracked_count > 0) {
        printf("Untracked files:\n");
        printf("  (use \"git add <file>...\" to include in what will be committed)\n");
        for (size_t i = 0; i < info->untracked_count; i++) {
            printf("\t%s\n", info->untracked[i]);
        }
        printf("\n");
    }

    // Print summary
    if (info->staged_count == 0 && info->modified_count == 0 &&
        info->untracked_count == 0) {
        printf("nothing to commit, working tree clean\n");
    } else if (info->staged_count == 0) {
        printf("nothing added to commit but untracked files present (use \"git add\" to track)\n");
    }

    status_info_free(info);
    git_index_free(index);
    return GIT_OK;
}
