#include "git_common.h"
#include "git_tree.h"
#include "git_sha1.h"
#include <stdio.h>

int cmd_write_tree(int argc, char *argv[]) {
    (void)argc;  // Unused
    (void)argv;  // Unused

    unsigned char sha1[SHA1_DIGEST_SIZE];
    int ret = git_tree_write_from_directory(".", sha1);

    if (ret != GIT_OK) {
        fprintf(stderr, "Failed to write tree: %s\n", git_error_last());
        return ret;
    }

    // Convert to hex and print
    char sha1_hex[SHA1_HEX_SIZE + 1];
    ret = git_sha1_to_hex(sha1, sha1_hex, sizeof(sha1_hex));
    if (ret != GIT_OK) {
        fprintf(stderr, "Failed to convert SHA-1: %s\n", git_error_last());
        return ret;
    }

    printf("%s\n", sha1_hex);
    return GIT_OK;
}
