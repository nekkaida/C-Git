/**
 * @file fuzz_sha1.c
 * @brief Fuzz testing for SHA-1 implementation
 *
 * Build with: clang -fsanitize=fuzzer,address -o fuzz_sha1 fuzz_sha1.c \
 *             ../src/core/sha1.c ../src/utils/error.c -I../include
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "git_sha1.h"
#include "git_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    git_sha1_ctx ctx;
    unsigned char digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    // Test basic hashing
    git_sha1_init(&ctx);
    git_sha1_update(&ctx, data, size);
    git_sha1_final(digest, &ctx);

    // Test hex conversion
    git_sha1_to_hex(digest, hex, sizeof(hex));

    // Test incremental updates (split data randomly)
    if (size > 1) {
        git_sha1_init(&ctx);
        size_t split = size / 2;
        git_sha1_update(&ctx, data, split);
        git_sha1_update(&ctx, data + split, size - split);
        git_sha1_final(digest, &ctx);
    }

    return 0;
}
