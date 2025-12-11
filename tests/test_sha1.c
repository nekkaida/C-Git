/**
 * @file test_sha1.c
 * @brief Unit tests for SHA-1 implementation
 */

#include "test_framework.h"
#include "../include/git_sha1.h"
#include "../include/git_common.h"
#include <string.h>

// Known SHA-1 test vectors from FIPS 180-2
void test_sha1_empty(void) {
    git_sha1_ctx ctx;
    unsigned char digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    git_sha1_init(&ctx);
    git_sha1_final(digest, &ctx);
    git_sha1_to_hex(digest, hex, sizeof(hex));

    // SHA-1 of empty string
    ASSERT_STR_EQ(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                  "SHA-1 of empty string");
}

void test_sha1_abc(void) {
    git_sha1_ctx ctx;
    unsigned char digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    git_sha1_init(&ctx);
    git_sha1_update(&ctx, "abc", 3);
    git_sha1_final(digest, &ctx);
    git_sha1_to_hex(digest, hex, sizeof(hex));

    // SHA-1 of "abc"
    ASSERT_STR_EQ(hex, "a9993e364706816aba3e25717850c26c9cd0d89d",
                  "SHA-1 of 'abc'");
}

void test_sha1_longer_string(void) {
    git_sha1_ctx ctx;
    unsigned char digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    const char *msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    git_sha1_init(&ctx);
    git_sha1_update(&ctx, msg, strlen(msg));
    git_sha1_final(digest, &ctx);
    git_sha1_to_hex(digest, hex, sizeof(hex));

    // SHA-1 of the 448-bit test message
    ASSERT_STR_EQ(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1",
                  "SHA-1 of 448-bit message");
}

void test_sha1_incremental(void) {
    git_sha1_ctx ctx;
    unsigned char digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    git_sha1_init(&ctx);
    git_sha1_update(&ctx, "a", 1);
    git_sha1_update(&ctx, "b", 1);
    git_sha1_update(&ctx, "c", 1);
    git_sha1_final(digest, &ctx);
    git_sha1_to_hex(digest, hex, sizeof(hex));

    // Should be same as hashing "abc" at once
    ASSERT_STR_EQ(hex, "a9993e364706816aba3e25717850c26c9cd0d89d",
                  "SHA-1 incremental update");
}

void test_sha1_hex_conversion(void) {
    unsigned char bytes[SHA1_DIGEST_SIZE] = {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
        0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
    };
    char hex[SHA1_HEX_SIZE + 1];
    unsigned char bytes_back[SHA1_DIGEST_SIZE];

    int ret = git_sha1_to_hex(bytes, hex, sizeof(hex));
    ASSERT_EQ(ret, GIT_OK, "git_sha1_to_hex returns GIT_OK");
    ASSERT_STR_EQ(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                  "Bytes to hex conversion");

    ret = git_sha1_from_hex(hex, bytes_back);
    ASSERT_EQ(ret, GIT_OK, "git_sha1_from_hex returns GIT_OK");
    ASSERT_MEM_EQ(bytes, bytes_back, SHA1_DIGEST_SIZE,
                  "Hex to bytes roundtrip");
}

void test_sha1_invalid_hex(void) {
    unsigned char bytes[SHA1_DIGEST_SIZE];

    // Too short
    int ret = git_sha1_from_hex("da39a3ee", bytes);
    ASSERT_NE(ret, GIT_OK, "Reject short hex string");

    // Invalid characters
    ret = git_sha1_from_hex("gg39a3ee5e6b4b0d3255bfef95601890afd80709", bytes);
    ASSERT_NE(ret, GIT_OK, "Reject invalid hex characters");

    // NULL input
    ret = git_sha1_from_hex(NULL, bytes);
    ASSERT_NE(ret, GIT_OK, "Reject NULL input");
}

void test_sha1_buffer_too_small(void) {
    unsigned char bytes[SHA1_DIGEST_SIZE] = {0};
    char small_buf[10];

    int ret = git_sha1_to_hex(bytes, small_buf, sizeof(small_buf));
    ASSERT_NE(ret, GIT_OK, "Reject too small buffer");
}

int main(void) {
    printf("# SHA-1 Unit Tests\n");
    // Test counts: empty(1) + abc(1) + longer(1) + incremental(1) +
    // hex_conversion(4) + invalid_hex(3) + buffer_too_small(1) = 12
    TAP_PLAN(12);

    RUN_TEST(test_sha1_empty);
    RUN_TEST(test_sha1_abc);
    RUN_TEST(test_sha1_longer_string);
    RUN_TEST(test_sha1_incremental);
    RUN_TEST(test_sha1_hex_conversion);
    RUN_TEST(test_sha1_invalid_hex);
    RUN_TEST(test_sha1_buffer_too_small);

    TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
