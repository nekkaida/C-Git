#ifndef GIT_SHA1_H
#define GIT_SHA1_H

#include "git_common.h"

// SHA-1 context structure
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[SHA1_BLOCK_SIZE];
} git_sha1_ctx;

// SHA-1 functions
void git_sha1_init(git_sha1_ctx *ctx);
void git_sha1_update(git_sha1_ctx *ctx, const void *data, size_t len);
void git_sha1_final(unsigned char digest[SHA1_DIGEST_SIZE], git_sha1_ctx *ctx);

// Convenience function
void git_sha1_hash(const void *data, size_t len, unsigned char digest[SHA1_DIGEST_SIZE]);

// Hash conversion utilities
int git_sha1_to_hex(const unsigned char *hash, char *hex, size_t hex_size);
int git_sha1_from_hex(const char *hex, unsigned char *hash);

#endif // GIT_SHA1_H
