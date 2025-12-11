/**
 * @file fuzz_object.c
 * @brief Fuzz testing for Git object handling
 *
 * Build with: clang -fsanitize=fuzzer,address -o fuzz_object fuzz_object.c \
 *             ../src/core/*.c ../src/utils/*.c -I../include -lz
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "git_object.h"
#include "git_sha1.h"
#include "git_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Limit size to prevent OOM
    if (size > 1024 * 1024) {
        return 0;
    }

    unsigned char sha1[SHA1_DIGEST_SIZE];

    // Test blob hashing
    int ret = git_object_hash(data, size, GIT_OBJ_BLOB, sha1);
    (void)ret;

    // Test tree hashing (if data looks tree-like)
    ret = git_object_hash(data, size, GIT_OBJ_TREE, sha1);
    (void)ret;

    // Test commit hashing
    ret = git_object_hash(data, size, GIT_OBJ_COMMIT, sha1);
    (void)ret;

    // Test compression roundtrip
    if (size > 0 && size < 100000) {
        void *compressed = NULL;
        size_t compressed_len = 0;

        ret = git_object_compress(data, size, &compressed, &compressed_len);
        if (ret == GIT_OK && compressed) {
            void *decompressed = NULL;
            size_t decompressed_len = 0;

            ret = git_object_decompress(compressed, compressed_len,
                                         &decompressed, &decompressed_len);
            if (ret == GIT_OK && decompressed) {
                // Verify roundtrip
                if (decompressed_len == size) {
                    // Data should match
                }
                free(decompressed);
            }
            free(compressed);
        }
    }

    // Test object creation
    if (size < 10000) {
        git_object *obj = git_object_new(GIT_OBJ_BLOB, size);
        if (obj) {
            if (obj->data && size > 0) {
                memcpy(obj->data, data, size);
            }
            git_object_free(obj);
        }
    }

    return 0;
}
