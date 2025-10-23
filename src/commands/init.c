#include "git_common.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>  // For _mkdir on Windows
#else
    #include <sys/stat.h>
#endif

// Helper function to create directory with proper error handling
static int create_dir(const char *path) {
    int result;
#ifdef _WIN32
    result = _mkdir(path);
    // On Windows, EEXIST or EACCES can mean directory exists
    if (result == -1 && errno != EEXIST && errno != EACCES) {
        // Double-check: try to access the directory
        struct _stat st;
        if (_stat(path, &st) == 0 && (st.st_mode & _S_IFDIR)) {
            return 0;  // Directory exists, success
        }
        return -1;
    }
#else
    result = mkdir(path, 0755);
    if (result == -1 && errno != EEXIST) {
        return -1;
    }
#endif
    return 0;
}

int cmd_init(int argc, char *argv[]) {
    (void)argc;  // Unused
    (void)argv;  // Unused

    // Create .git directory structure
    if (create_dir(".git") == -1) {
        fprintf(stderr, "Failed to create .git directory: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (create_dir(".git/objects") == -1) {
        fprintf(stderr, "Failed to create .git/objects directory: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (create_dir(".git/refs") == -1) {
        fprintf(stderr, "Failed to create .git/refs directory: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (create_dir(".git/refs/heads") == -1) {
        fprintf(stderr, "Failed to create .git/refs/heads directory: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (create_dir(".git/refs/tags") == -1) {
        fprintf(stderr, "Failed to create .git/refs/tags directory: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    // Create HEAD file
    FILE *head_file = fopen(".git/HEAD", "w");
    if (head_file == NULL) {
        fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (fprintf(head_file, "ref: refs/heads/main\n") < 0) {
        fclose(head_file);
        fprintf(stderr, "Failed to write to .git/HEAD file: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    if (fclose(head_file) != 0) {
        fprintf(stderr, "Failed to close .git/HEAD file: %s\n", strerror(errno));
        return GIT_ERROR;
    }

    printf("Initialized git directory\n");
    return GIT_OK;
}
