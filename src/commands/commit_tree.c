#include "git_common.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int cmd_commit_tree(int argc, char *argv[]) {
    const char *tree_sha = NULL;
    const char *parent_sha = NULL;
    const char *message = NULL;

    // Parse arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: commit-tree <tree-sha> [-p <parent-commit>] -m <message>\n");
        return GIT_EINVALID;
    }

    tree_sha = argv[2];

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            parent_sha = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[i + 1];
            i++;
        }
    }

    if (tree_sha == NULL || message == NULL) {
        fprintf(stderr, "Missing required arguments\n");
        return GIT_EINVALID;
    }

    // Validate message size
    if (strlen(message) > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Commit message too long (max %d bytes)\n", MAX_MESSAGE_SIZE);
        return GIT_EINVALID;
    }

    // Validate tree SHA
    if (!git_validate_sha1_hex(tree_sha)) {
        fprintf(stderr, "Invalid tree SHA-1: %s\n", tree_sha);
        return GIT_EINVALID;
    }

    // Validate parent SHA if provided
    if (parent_sha != NULL && !git_validate_sha1_hex(parent_sha)) {
        fprintf(stderr, "Invalid parent commit SHA-1: %s\n", parent_sha);
        return GIT_EINVALID;
    }

    // Hardcoded author/committer info (TODO: read from config)
    const char *author = "Example Author <author@example.com>";
    const char *committer = "Example Committer <committer@example.com>";

    // Get current timestamp
    time_t now = time(NULL);

    // Get timezone offset (cross-platform)
    struct tm *tm_local = localtime(&now);
    struct tm tm_local_copy = *tm_local;  // Copy before calling gmtime

    // Calculate offset in seconds
    long tz_offset = 0;

#if defined(_WIN32)
    // Windows: use _get_timezone
    long timezone_seconds = 0;
    _get_timezone(&timezone_seconds);
    tz_offset = -timezone_seconds;
    if (tm_local_copy.tm_isdst > 0) {
        tz_offset += 3600;  // Add DST hour
    }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    // BSD systems: tm_gmtoff is available
    tz_offset = tm_local_copy.tm_gmtoff;
#elif defined(__linux__) || defined(__GLIBC__)
    // Linux/glibc: use __tm_gmtoff (glibc internal name)
    tz_offset = tm_local_copy.__tm_gmtoff;
#else
    // Fallback: calculate from difference between local and UTC
    // Note: mktime() treats tm as local time, so we calculate offset differently
    struct tm *tm_utc = gmtime(&now);
    struct tm tm_utc_copy = *tm_utc;
    tm_utc_copy.tm_isdst = 0;  // UTC has no DST
    // For systems without tm_gmtoff, approximate using localtime difference
    // This is not perfect but works for most cases
    tz_offset = (long)difftime(now, mktime(&tm_utc_copy));
    (void)tm_local_copy;  // Suppress unused warning
#endif

    int tz_hours = (int)(tz_offset / 3600);
    int tz_mins = (int)(labs(tz_offset % 3600) / 60);
    char tz_str[10];
    int tz_ret = snprintf(tz_str, sizeof(tz_str), "%+03d%02d", tz_hours, tz_mins);
    if (tz_ret < 0 || tz_ret >= (int)sizeof(tz_str)) {
        fprintf(stderr, "Failed to format timezone string\n");
        return GIT_ERROR;
    }

    // Calculate buffer size needed - with overflow protection
    size_t buffer_size = 1024;  // Start with reasonable size

    size_t tree_len = strlen(tree_sha);
    size_t parent_len = parent_sha ? strlen(parent_sha) : 0;
    size_t author_len = strlen(author);
    size_t committer_len = strlen(committer);
    size_t message_len = strlen(message);

    // Check for overflow in individual additions
    size_t additions = tree_len + author_len + committer_len + message_len + 120;
    if (parent_sha) {
        if (additions > SIZE_MAX - parent_len - 10) {
            fprintf(stderr, "Commit content size would overflow\n");
            return GIT_EOVERFLOW;
        }
        additions += parent_len + 10;
    }

    if (buffer_size > SIZE_MAX - additions) {
        fprintf(stderr, "Commit buffer size would overflow\n");
        return GIT_EOVERFLOW;
    }
    buffer_size += additions;

    char *commit_content = (char *)malloc(buffer_size);
    if (commit_content == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return GIT_ENOMEM;
    }

    // Build commit content
    int content_len = 0;

    // Tree
    content_len += snprintf(commit_content + content_len,
                           buffer_size - content_len,
                           "tree %s\n", tree_sha);

    // Parent (if provided)
    if (parent_sha != NULL) {
        content_len += snprintf(commit_content + content_len,
                               buffer_size - content_len,
                               "parent %s\n", parent_sha);
    }

    // Author
    content_len += snprintf(commit_content + content_len,
                           buffer_size - content_len,
                           "author %s %ld %s\n",
                           author, (long)now, tz_str);

    // Committer
    content_len += snprintf(commit_content + content_len,
                           buffer_size - content_len,
                           "committer %s %ld %s\n",
                           committer, (long)now, tz_str);

    // Blank line + message
    content_len += snprintf(commit_content + content_len,
                           buffer_size - content_len,
                           "\n%s\n", message);

    // Create commit object
    git_object *obj = git_object_new(GIT_OBJ_COMMIT, content_len);
    if (!obj) {
        free(commit_content);
        fprintf(stderr, "Failed to create commit object: %s\n", git_error_last());
        return GIT_ENOMEM;
    }

    memcpy(obj->data, commit_content, content_len);
    free(commit_content);

    // Write commit
    int ret = git_object_write(obj);
    if (ret != GIT_OK) {
        git_object_free(obj);
        fprintf(stderr, "Failed to write commit: %s\n", git_error_last());
        return ret;
    }

    // Print commit SHA
    char sha1_hex[SHA1_HEX_SIZE + 1];
    ret = git_sha1_to_hex(obj->sha1, sha1_hex, sizeof(sha1_hex));
    if (ret != GIT_OK) {
        git_object_free(obj);
        fprintf(stderr, "Failed to convert SHA-1: %s\n", git_error_last());
        return ret;
    }

    printf("%s\n", sha1_hex);

    git_object_free(obj);
    return GIT_OK;
}
