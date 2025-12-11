/**
 * @file main_refactored.c
 * @brief C-Git entry point and command dispatcher
 *
 * TODO(Phase 2): New commands to implement
 *   - git branch: Create, list, delete branches
 *   - git checkout: Switch branches, restore files
 *   - git merge: Merge branches (fast-forward first)
 *   - git diff: Show changes between commits/working tree
 *   - git rm: Remove files from index and working tree
 *   - git mv: Move/rename files
 *
 * TODO(Phase 3): Remote commands
 *   - git remote: Manage remote repositories
 *   - git fetch: Download objects from remote
 *   - git push: Upload objects to remote
 *   - git pull: Fetch and merge
 *
 * TODO(Phase 3): Advanced commands
 *   - git stash: Save working directory state
 *   - git tag: Create and manage tags
 *   - git reset: Reset HEAD to a state
 *   - git revert: Create commit that undoes changes
 *
 * TODO(Future): Porcelain improvements
 *   - Pager support (pipe to less)
 *   - Config file parsing (.gitconfig)
 *   - Alias support
 *   - Shell completion helpers
 */

#include "git_common.h"
#include <stdio.h>
#include <string.h>

// Command function prototypes
extern int cmd_init(int argc, char *argv[]);
extern int cmd_hash_object(int argc, char *argv[]);
extern int cmd_cat_file(int argc, char *argv[]);
extern int cmd_ls_tree(int argc, char *argv[]);
extern int cmd_write_tree(int argc, char *argv[]);
extern int cmd_commit_tree(int argc, char *argv[]);
extern int cmd_add(int argc, char *argv[]);
extern int cmd_status(int argc, char *argv[]);
extern int cmd_log(int argc, char *argv[]);

// Print usage information
static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s <command> [<args>]\n\n", program);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  init                     Initialize a new Git repository\n");
    fprintf(stderr, "  add <file>...            Add file contents to the index\n");
    fprintf(stderr, "  status                   Show the working tree status\n");
    fprintf(stderr, "  log [--oneline] [-n N]   Show commit logs\n");
    fprintf(stderr, "  hash-object [-w] <file>  Compute object ID and optionally create a blob\n");
    fprintf(stderr, "  cat-file -p <object>     Provide content of repository objects\n");
    fprintf(stderr, "  ls-tree [--name-only] <tree-sha>\n");
    fprintf(stderr, "                           List the contents of a tree object\n");
    fprintf(stderr, "  write-tree               Create a tree object from the current directory\n");
    fprintf(stderr, "  commit-tree <tree> [-p <parent>] -m <message>\n");
    fprintf(stderr, "                           Create a new commit object\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help               Show this help message\n");
    fprintf(stderr, "  --version                Show version information\n");
}

// Print version information
static void print_version(void) {
    printf("C-Git version 0.4.0 (Production Readiness Release)\n");
    printf("A lightweight educational Git implementation\n");
    printf("\n");
    printf("Built with:\n");
    printf("  - Direct zlib integration (no shell calls)\n");
    printf("  - Modular architecture with clean API\n");
    printf("  - OpenSSF-compliant compiler hardening\n");
    printf("  - Thread-safe error handling\n");
    printf("  - Index/staging area support\n");
    printf("  - Cross-platform support (Windows/macOS/Linux/BSD)\n");
    printf("\n");
    printf("Supported commands: init, add, status, log, hash-object,\n");
    printf("                    cat-file, ls-tree, write-tree, commit-tree\n");
}

int main(int argc, char *argv[]) {
    // Disable output buffering for better output consistency
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // Check for minimum arguments
    if (argc < 2) {
        print_usage(argv[0]);
        return GIT_EINVALID;
    }

    const char *command = argv[1];

    // Handle help and version flags
    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_usage(argv[0]);
        return GIT_OK;
    }

    if (strcmp(command, "--version") == 0) {
        print_version();
        return GIT_OK;
    }

    // Dispatch to appropriate command handler
    int result = GIT_ERROR;

    if (strcmp(command, "init") == 0) {
        result = cmd_init(argc, argv);
    }
    else if (strcmp(command, "hash-object") == 0) {
        result = cmd_hash_object(argc, argv);
    }
    else if (strcmp(command, "cat-file") == 0) {
        result = cmd_cat_file(argc, argv);
    }
    else if (strcmp(command, "ls-tree") == 0) {
        result = cmd_ls_tree(argc, argv);
    }
    else if (strcmp(command, "write-tree") == 0) {
        result = cmd_write_tree(argc, argv);
    }
    else if (strcmp(command, "commit-tree") == 0) {
        result = cmd_commit_tree(argc, argv);
    }
    else if (strcmp(command, "add") == 0) {
        result = cmd_add(argc, argv);
    }
    else if (strcmp(command, "status") == 0) {
        result = cmd_status(argc, argv);
    }
    else if (strcmp(command, "log") == 0) {
        result = cmd_log(argc, argv);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage(argv[0]);
        return GIT_EINVALID;
    }

    // Return appropriate exit code
    // Git-style: 0 for success, positive for errors
    return (result == GIT_OK) ? 0 : 1;
}
