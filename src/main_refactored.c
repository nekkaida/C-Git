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

// Print usage information
static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s <command> [<args>]\n\n", program);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  init                     Initialize a new Git repository\n");
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
    printf("C-Git version 0.3.5 (Security Hardening Release)\n");
    printf("A lightweight educational Git implementation\n");
    printf("\n");
    printf("Built with:\n");
    printf("  - Direct zlib integration\n");
    printf("  - Modular architecture\n");
    printf("  - Security-hardened input validation\n");
    printf("  - Cross-platform support (Windows/macOS/Linux/BSD)\n");
    printf("\n");
    printf("WARNING: Educational use only. Not for production.\n");
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
    else {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage(argv[0]);
        return GIT_EINVALID;
    }

    // Return appropriate exit code
    // Git-style: 0 for success, positive for errors
    return (result == GIT_OK) ? 0 : 1;
}
