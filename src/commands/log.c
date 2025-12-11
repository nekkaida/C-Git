/**
 * @file log.c
 * @brief Implementation of git log command
 *
 * TODO(Phase 1): Multiple parents (merge commits)
 *   - Parse all "parent" lines, not just first
 *   - Follow first parent by default
 *   - Add --first-parent flag
 *
 * TODO(Phase 1): Graph visualization
 *   - ASCII art commit graph (--graph)
 *   - Branch/merge visualization
 *
 * TODO(Phase 2): Filtering options
 *   - --author=<pattern>
 *   - --since/--until (date filtering)
 *   - --grep=<pattern> (message search)
 *   - -- <path> (path filtering)
 *
 * TODO(Phase 2): Format options
 *   - --format=<format> (custom format strings)
 *   - --pretty=short/medium/full/fuller
 *   - --stat (show changed files)
 *
 * TODO(Phase 3): Revision ranges
 *   - A..B (reachable from B, not from A)
 *   - A...B (symmetric difference)
 *   - ^A (exclude reachable from A)
 */

#include "git_common.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

// Check if output supports color
static int use_color(void) {
    return isatty(fileno(stdout));
}

// Maximum number of commits to show by default
#define DEFAULT_MAX_COMMITS 20

// Parse a commit object and extract its fields
typedef struct {
    char tree[SHA1_HEX_SIZE + 1];
    char parent[SHA1_HEX_SIZE + 1];
    char author_name[256];
    char author_email[256];
    time_t author_time;
    int author_tz;
    char committer_name[256];
    char committer_email[256];
    time_t committer_time;
    int committer_tz;
    char *message;
} commit_info;

static void commit_info_free(commit_info *info) {
    if (info && info->message) {
        free(info->message);
        info->message = NULL;
    }
}

static int parse_person(const char *line, char *name, size_t name_size,
                        char *email, size_t email_size,
                        time_t *when, int *tz) {
    // Format: "author Name <email> timestamp timezone"
    const char *lt = strchr(line, '<');
    const char *gt = strchr(line, '>');

    if (!lt || !gt || lt >= gt) return -1;

    // Name is between start and '<'
    size_t name_len = (size_t)(lt - line - 1);  // -1 for space
    if (name_len >= name_size) name_len = name_size - 1;
    strncpy(name, line, name_len);
    name[name_len] = '\0';

    // Email is between '<' and '>'
    size_t email_len = (size_t)(gt - lt - 1);
    if (email_len >= email_size) email_len = email_size - 1;
    strncpy(email, lt + 1, email_len);
    email[email_len] = '\0';

    // Timestamp and timezone after '>'
    const char *time_str = gt + 2;  // Skip "> "
    char *end;
    *when = (time_t)strtol(time_str, &end, 10);

    if (*end == ' ') {
        *tz = (int)strtol(end + 1, NULL, 10);
    } else {
        *tz = 0;
    }

    return 0;
}

static int parse_commit(git_object *obj, commit_info *info) {
    if (!obj || !info || obj->type != GIT_OBJ_COMMIT) {
        return -1;
    }

    memset(info, 0, sizeof(commit_info));

    const char *data = (const char *)obj->data;
    const char *end = data + obj->size;
    const char *line_start = data;

    int in_message = 0;
    char *message_start = NULL;

    while (line_start < end) {
        // Find end of line
        const char *line_end = memchr(line_start, '\n', (size_t)(end - line_start));
        if (!line_end) line_end = end;

        size_t line_len = (size_t)(line_end - line_start);

        if (in_message) {
            // Already in message - accumulate
            if (!message_start) {
                message_start = (char *)line_start;
            }
        } else if (line_len == 0) {
            // Empty line - start of message
            in_message = 1;
        } else if (strncmp(line_start, "tree ", 5) == 0 && line_len >= 45) {
            strncpy(info->tree, line_start + 5, SHA1_HEX_SIZE);
            info->tree[SHA1_HEX_SIZE] = '\0';
        } else if (strncmp(line_start, "parent ", 7) == 0 && line_len >= 47) {
            strncpy(info->parent, line_start + 7, SHA1_HEX_SIZE);
            info->parent[SHA1_HEX_SIZE] = '\0';
        } else if (strncmp(line_start, "author ", 7) == 0) {
            parse_person(line_start + 7,
                        info->author_name, sizeof(info->author_name),
                        info->author_email, sizeof(info->author_email),
                        &info->author_time, &info->author_tz);
        } else if (strncmp(line_start, "committer ", 10) == 0) {
            parse_person(line_start + 10,
                        info->committer_name, sizeof(info->committer_name),
                        info->committer_email, sizeof(info->committer_email),
                        &info->committer_time, &info->committer_tz);
        }

        line_start = line_end + 1;
    }

    // Copy message
    if (message_start && message_start < end) {
        size_t msg_len = (size_t)(end - message_start);
        info->message = (char *)malloc(msg_len + 1);
        if (info->message) {
            memcpy(info->message, message_start, msg_len);
            info->message[msg_len] = '\0';

            // Trim trailing newline
            while (msg_len > 0 && info->message[msg_len - 1] == '\n') {
                info->message[--msg_len] = '\0';
            }
        }
    }

    return 0;
}

static void format_time(time_t when, int tz, char *buf, size_t size) {
    struct tm *tm = localtime(&when);
    if (tm) {
        strftime(buf, size, "%a %b %d %H:%M:%S %Y", tm);

        // Append timezone
        size_t len = strlen(buf);
        if (len + 7 < size) {
            int tz_hours = tz / 100;
            int tz_mins = abs(tz % 100);
            snprintf(buf + len, size - len, " %+03d%02d", tz_hours, tz_mins);
        }
    } else {
        snprintf(buf, size, "(unknown time)");
    }
}

static int read_head_sha(char *sha, size_t size) {
    FILE *head = fopen(".git/HEAD", "r");
    if (!head) {
        return -1;
    }

    char line[256];
    if (!fgets(line, sizeof(line), head)) {
        fclose(head);
        return -1;
    }
    fclose(head);

    // Remove newline
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    if (strncmp(line, "ref: ", 5) == 0) {
        // Symbolic reference - read the ref file
        char ref_path[MAX_PATH_LEN];
        snprintf(ref_path, sizeof(ref_path), ".git/%s", line + 5);

        FILE *ref = fopen(ref_path, "r");
        if (!ref) {
            return -1;  // No commits yet
        }

        if (!fgets(sha, (int)size, ref)) {
            fclose(ref);
            return -1;
        }
        fclose(ref);

        nl = strchr(sha, '\n');
        if (nl) *nl = '\0';
    } else {
        // Direct SHA (detached HEAD)
        strncpy(sha, line, size - 1);
        sha[size - 1] = '\0';
    }

    return 0;
}

static void print_commit(const char *sha, const commit_info *info, int oneline) {
    int color = use_color();
    const char *yellow = color ? "\033[33m" : "";
    const char *reset = color ? "\033[0m" : "";

    if (oneline) {
        // Short format: sha message-first-line
        const char *first_newline = info->message ? strchr(info->message, '\n') : NULL;
        if (first_newline) {
            printf("%s%.7s%s %.*s\n", yellow, sha, reset,
                   (int)(first_newline - info->message), info->message);
        } else {
            printf("%s%.7s%s %s\n", yellow, sha, reset,
                   info->message ? info->message : "(no message)");
        }
    } else {
        // Full format
        printf("%scommit %s%s\n", yellow, sha, reset);
        printf("Author: %s <%s>\n", info->author_name, info->author_email);

        char time_buf[64];
        format_time(info->author_time, info->author_tz, time_buf, sizeof(time_buf));
        printf("Date:   %s\n", time_buf);

        printf("\n");
        if (info->message) {
            // Indent message lines
            const char *line = info->message;
            while (*line) {
                const char *next = strchr(line, '\n');
                if (next) {
                    printf("    %.*s\n", (int)(next - line), line);
                    line = next + 1;
                } else {
                    printf("    %s\n", line);
                    break;
                }
            }
        }
        printf("\n");
    }
}

int cmd_log(int argc, char *argv[]) {
    int max_commits = DEFAULT_MAX_COMMITS;
    int oneline = 0;

    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--oneline") == 0) {
            oneline = 1;
        } else if (strncmp(argv[i], "-n", 2) == 0) {
            if (argv[i][2] != '\0') {
                max_commits = atoi(argv[i] + 2);
            } else if (i + 1 < argc) {
                max_commits = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-1") == 0) {
            max_commits = 1;
        }
    }

    // Get HEAD commit
    char current_sha[SHA1_HEX_SIZE + 1];
    if (read_head_sha(current_sha, sizeof(current_sha)) != 0) {
        printf("fatal: your current branch does not have any commits yet\n");
        return GIT_ERROR;
    }

    if (!git_validate_sha1_hex(current_sha)) {
        printf("fatal: bad HEAD reference\n");
        return GIT_ERROR;
    }

    // Walk commit history
    int count = 0;
    while (count < max_commits && current_sha[0] != '\0') {
        // Read commit object
        git_object *obj = NULL;
        int ret = git_object_read(current_sha, &obj);
        if (ret != GIT_OK) {
            if (count == 0) {
                fprintf(stderr, "fatal: bad object %s\n", current_sha);
                return GIT_ERROR;
            }
            break;
        }

        if (obj->type != GIT_OBJ_COMMIT) {
            git_object_free(obj);
            fprintf(stderr, "fatal: object %s is not a commit\n", current_sha);
            return GIT_ERROR;
        }

        // Parse commit
        commit_info info;
        if (parse_commit(obj, &info) != 0) {
            git_object_free(obj);
            fprintf(stderr, "fatal: cannot parse commit %s\n", current_sha);
            return GIT_ERROR;
        }

        // Print commit
        print_commit(current_sha, &info, oneline);
        count++;

        // Move to parent
        if (info.parent[0] != '\0') {
            strncpy(current_sha, info.parent, SHA1_HEX_SIZE);
            current_sha[SHA1_HEX_SIZE] = '\0';
        } else {
            current_sha[0] = '\0';  // No more parents
        }

        commit_info_free(&info);
        git_object_free(obj);
    }

    return GIT_OK;
}
