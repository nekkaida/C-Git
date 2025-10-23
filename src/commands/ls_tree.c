#include "git_common.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdio.h>
#include <string.h>

int cmd_ls_tree(int argc, char *argv[]) {
    int name_only = 0;
    const char *tree_sha = NULL;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--name-only") == 0) {
            name_only = 1;
        } else {
            tree_sha = argv[i];
        }
    }

    if (tree_sha == NULL) {
        fprintf(stderr, "Usage: ls-tree [--name-only] <tree-sha>\n");
        return GIT_EINVALID;
    }

    // SECURITY: Validate SHA-1 hash format
    if (!git_validate_sha1_hex(tree_sha)) {
        fprintf(stderr, "Invalid tree hash (must be 40 hex characters): %s\n", tree_sha);
        return GIT_EINVALID;
    }

    // Read tree object
    git_object *obj = NULL;
    int ret = git_object_read(tree_sha, &obj);
    if (ret != GIT_OK) {
        fprintf(stderr, "Failed to read tree: %s\n", git_error_last());
        return ret;
    }

    if (obj->type != GIT_OBJ_TREE) {
        fprintf(stderr, "Object %s is not a tree\n", tree_sha);
        git_object_free(obj);
        return GIT_EINVALID;
    }

    // Parse tree entries
    unsigned char *data = (unsigned char *)obj->data;
    size_t offset = 0;

    while (offset < obj->size) {
        // Read mode
        char mode[10];
        size_t mode_idx = 0;

        while (offset < obj->size && data[offset] != ' ' && mode_idx < sizeof(mode) - 1) {
            mode[mode_idx++] = data[offset++];
        }
        mode[mode_idx] = '\0';

        // Validate that we found a space
        if (offset >= obj->size || data[offset] != ' ') {
            fprintf(stderr, "Malformed tree object: missing space after mode\n");
            git_object_free(obj);
            return GIT_ERROR;
        }
        offset++;  // Skip space

        // Read name
        char name[256];
        size_t name_idx = 0;

        while (offset < obj->size && data[offset] != '\0' && name_idx < sizeof(name) - 1) {
            name[name_idx++] = data[offset++];
        }
        name[name_idx] = '\0';

        // Validate that we found a null byte
        if (offset >= obj->size || data[offset] != '\0') {
            fprintf(stderr, "Malformed tree object: missing null byte after name\n");
            git_object_free(obj);
            return GIT_ERROR;
        }
        offset++;  // Skip null byte

        // Read SHA-1 (20 bytes)
        if (offset + SHA1_DIGEST_SIZE > obj->size) {
            fprintf(stderr, "Malformed tree object: truncated SHA-1\n");
            git_object_free(obj);
            return GIT_ERROR;
        }

        unsigned char sha1_bytes[SHA1_DIGEST_SIZE];
        memcpy(sha1_bytes, data + offset, SHA1_DIGEST_SIZE);
        offset += SHA1_DIGEST_SIZE;

        // Convert SHA-1 to hex
        char sha1_hex[SHA1_HEX_SIZE + 1];
        if (git_sha1_to_hex(sha1_bytes, sha1_hex, sizeof(sha1_hex)) != GIT_OK) {
            fprintf(stderr, "Failed to convert SHA-1 to hex\n");
            git_object_free(obj);
            return GIT_ERROR;
        }

        // Determine type based on mode
        const char *type = (strcmp(mode, "40000") == 0 || strcmp(mode, "040000") == 0) ? "tree" : "blob";

        // Print based on --name-only flag
        if (name_only) {
            printf("%s\n", name);
        } else {
            // Format mode with leading zero if needed
            if (strlen(mode) == 5) {
                printf("0%s %s %s\t%s\n", mode, type, sha1_hex, name);
            } else {
                printf("%s %s %s\t%s\n", mode, type, sha1_hex, name);
            }
        }
    }

    git_object_free(obj);
    return GIT_OK;
}
