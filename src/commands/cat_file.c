#include "git_common.h"
#include "git_object.h"
#include "git_validation.h"
#include <stdio.h>
#include <string.h>

int cmd_cat_file(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: cat-file [-p] <object>\n");
        return GIT_EINVALID;
    }

    if (strcmp(argv[2], "-p") != 0) {
        fprintf(stderr, "Only -p option is supported\n");
        return GIT_EINVALID;
    }

    const char *hash = argv[3];

    // SECURITY: Validate SHA-1 hash format
    if (!git_validate_sha1_hex(hash)) {
        fprintf(stderr, "Invalid object hash (must be 40 hex characters): %s\n", hash);
        return GIT_EINVALID;
    }

    // Read object
    git_object *obj = NULL;
    int ret = git_object_read(hash, &obj);
    if (ret != GIT_OK) {
        fprintf(stderr, "Failed to read object: %s\n", git_error_last());
        return ret;
    }

    // Print content (for blob, just print the data)
    if (obj->type == GIT_OBJ_BLOB) {
        size_t written = fwrite(obj->data, 1, obj->size, stdout);
        if (written != obj->size) {
            fprintf(stderr, "Failed to write blob data: expected %zu bytes, wrote %zu bytes\n",
                    obj->size, written);
            git_object_free(obj);
            return GIT_ERROR;
        }
    } else if (obj->type == GIT_OBJ_TREE) {
        // For tree, print formatted output
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

            unsigned char sha1[SHA1_DIGEST_SIZE];
            memcpy(sha1, data + offset, SHA1_DIGEST_SIZE);
            offset += SHA1_DIGEST_SIZE;

            // Convert SHA-1 to hex
            char sha1_hex[SHA1_HEX_SIZE + 1];
            if (git_sha1_to_hex(sha1, sha1_hex, sizeof(sha1_hex)) != GIT_OK) {
                fprintf(stderr, "Failed to convert SHA-1 to hex\n");
                git_object_free(obj);
                return GIT_ERROR;
            }

            printf("%s %s\n", mode, sha1_hex);
        }
    } else if (obj->type == GIT_OBJ_COMMIT) {
        // For commit, just print the data
        size_t written = fwrite(obj->data, 1, obj->size, stdout);
        if (written != obj->size) {
            fprintf(stderr, "Failed to write commit data: expected %zu bytes, wrote %zu bytes\n",
                    obj->size, written);
            git_object_free(obj);
            return GIT_ERROR;
        }
    } else {
        fprintf(stderr, "Unsupported object type\n");
        git_object_free(obj);
        return GIT_ERROR;
    }

    git_object_free(obj);
    return GIT_OK;
}
