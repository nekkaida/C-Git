#include "git_sha1.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    | (rol(block->l[i],8)&0x00FF00FF))

#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

typedef union {
    unsigned char c[64];
    uint32_t l[16];
} CHAR64LONG16;

static void sha1_transform(uint32_t state[5], const unsigned char buffer[64]) {
    uint32_t a, b, c, d, e;
    CHAR64LONG16 block[1];

    memcpy(block, buffer, 64);

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void git_sha1_init(git_sha1_ctx *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
}

void git_sha1_update(git_sha1_ctx *ctx, const void *data, size_t len) {
    const unsigned char *d = (const unsigned char *)data;
    uint32_t i, j;

    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += (uint32_t)(len << 3)) < (uint32_t)(len << 3))
        ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);

    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], d, (i = 64-j));
        sha1_transform(ctx->state, ctx->buffer);
        for ( ; i + 63 < len; i += 64) {
            sha1_transform(ctx->state, d + i);
        }
        j = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[j], &d[i], len - i);
}

void git_sha1_final(unsigned char digest[SHA1_DIGEST_SIZE], git_sha1_ctx *ctx) {
    uint32_t i;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((ctx->count[(i >= 4 ? 0 : 1)]
            >> ((3-(i & 3)) * 8) ) & 255);
    }

    git_sha1_update(ctx, (unsigned char *)"\200", 1);
    while ((ctx->count[0] & 504) != 448) {
        git_sha1_update(ctx, (unsigned char *)"\0", 1);
    }
    git_sha1_update(ctx, finalcount, 8);

    for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
        digest[i] = (unsigned char)
            ((ctx->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
}

void git_sha1_hash(const void *data, size_t len, unsigned char digest[SHA1_DIGEST_SIZE]) {
    git_sha1_ctx ctx;
    git_sha1_init(&ctx);
    git_sha1_update(&ctx, data, len);
    git_sha1_final(digest, &ctx);
}

int git_sha1_to_hex(const unsigned char *hash, char *hex, size_t hex_size) {
    if (!hash || !hex || hex_size < (SHA1_HEX_SIZE + 1)) {
        return GIT_EINVALID;
    }

    const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        hex[i * 2] = hex_chars[(hash[i] >> 4) & 0xF];
        hex[i * 2 + 1] = hex_chars[hash[i] & 0xF];
    }
    hex[SHA1_HEX_SIZE] = '\0';
    return GIT_OK;
}

int git_sha1_from_hex(const char *hex, unsigned char *hash) {
    if (!hex || !hash) {
        return GIT_EINVALID;
    }

    // Enforce precondition: hex must be exactly 40 characters
    size_t len = strlen(hex);
    if (len != SHA1_HEX_SIZE) {
        return GIT_EINVALID;
    }

    // Validate all characters are hexadecimal
    for (size_t i = 0; i < SHA1_HEX_SIZE; i++) {
        if (!isxdigit((unsigned char)hex[i])) {
            return GIT_EINVALID;
        }
    }

    // Now safe to parse
    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        unsigned int byte;
        if (sscanf(&hex[i*2], "%2x", &byte) != 1) {
            return GIT_EINVALID;
        }
        hash[i] = (unsigned char)byte;
    }
    return GIT_OK;
}
