/**
 * @file fuzz_validation.c
 * @brief Fuzz testing for input validation functions
 *
 * Build with: clang -fsanitize=fuzzer,address -o fuzz_validation fuzz_validation.c \
 *             ../src/utils/validation.c ../src/utils/error.c -I../include
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "git_validation.h"
#include "git_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Create null-terminated string from fuzzer input
    char *str = (char *)malloc(size + 1);
    if (!str) return 0;

    memcpy(str, data, size);
    str[size] = '\0';

    // Test SHA-1 validation
    git_validate_sha1_hex(str);

    // Test path validation
    git_validate_safe_path(str);

    // Test mode validation
    git_validate_mode(str);

    // Test safe string copy
    if (size > 0) {
        char dest[256];
        git_safe_strncpy(dest, str, sizeof(dest));
    }

    // Test path join
    if (size > 1) {
        char dest[512];
        size_t half = size / 2;
        char *base = (char *)malloc(half + 1);
        char *path = (char *)malloc(size - half + 1);

        if (base && path) {
            memcpy(base, data, half);
            base[half] = '\0';
            memcpy(path, data + half, size - half);
            path[size - half] = '\0';

            git_safe_path_join(dest, sizeof(dest), base, path);
        }

        free(base);
        free(path);
    }

    free(str);
    return 0;
}
