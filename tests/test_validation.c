/**
 * @file test_validation.c
 * @brief Unit tests for input validation functions
 */

#include "test_framework.h"
#include "../include/git_validation.h"
#include "../include/git_common.h"

void test_validate_sha1_hex_valid(void) {
    ASSERT_TRUE(git_validate_sha1_hex("da39a3ee5e6b4b0d3255bfef95601890afd80709"),
                "Valid SHA-1 lowercase");
    ASSERT_TRUE(git_validate_sha1_hex("DA39A3EE5E6B4B0D3255BFEF95601890AFD80709"),
                "Valid SHA-1 uppercase");
    ASSERT_TRUE(git_validate_sha1_hex("Da39A3Ee5e6B4b0D3255BfEf95601890AfD80709"),
                "Valid SHA-1 mixed case");
}

void test_validate_sha1_hex_invalid(void) {
    ASSERT_FALSE(git_validate_sha1_hex(NULL), "NULL SHA-1 rejected");
    ASSERT_FALSE(git_validate_sha1_hex(""), "Empty SHA-1 rejected");
    ASSERT_FALSE(git_validate_sha1_hex("da39a3ee"), "Short SHA-1 rejected");
    ASSERT_FALSE(git_validate_sha1_hex("gg39a3ee5e6b4b0d3255bfef95601890afd80709"),
                 "Invalid chars rejected");
    ASSERT_FALSE(git_validate_sha1_hex("da39a3ee5e6b4b0d3255bfef95601890afd80709x"),
                 "Too long SHA-1 rejected");
}

void test_validate_safe_path_valid(void) {
    ASSERT_TRUE(git_validate_safe_path("file.txt"), "Simple filename");
    ASSERT_TRUE(git_validate_safe_path("dir/file.txt"), "Path with directory");
    ASSERT_TRUE(git_validate_safe_path("a/b/c/d.txt"), "Deep path");
    ASSERT_TRUE(git_validate_safe_path("file-name_123.c"), "Special chars in name");
}

void test_validate_safe_path_invalid(void) {
    ASSERT_FALSE(git_validate_safe_path(NULL), "NULL path rejected");
    ASSERT_FALSE(git_validate_safe_path(""), "Empty path rejected");
    ASSERT_FALSE(git_validate_safe_path("../etc/passwd"), "Parent traversal rejected");
    ASSERT_FALSE(git_validate_safe_path("/etc/passwd"), "Absolute Unix path rejected");
    ASSERT_FALSE(git_validate_safe_path("\\Windows\\System32"), "Absolute Windows path rejected");
    ASSERT_FALSE(git_validate_safe_path("dir/../file"), "Hidden traversal rejected");
    ASSERT_FALSE(git_validate_safe_path(".."), "Double dot rejected");
}

void test_validate_mode_valid(void) {
    ASSERT_TRUE(git_validate_mode("040000"), "Directory mode");
    ASSERT_TRUE(git_validate_mode("100644"), "Regular file mode");
    ASSERT_TRUE(git_validate_mode("100755"), "Executable mode");
    ASSERT_TRUE(git_validate_mode("120000"), "Symlink mode");
    ASSERT_TRUE(git_validate_mode("160000"), "Gitlink mode");
}

void test_validate_mode_invalid(void) {
    ASSERT_FALSE(git_validate_mode(NULL), "NULL mode rejected");
    ASSERT_FALSE(git_validate_mode(""), "Empty mode rejected");
    ASSERT_FALSE(git_validate_mode("100666"), "Invalid mode rejected");
    ASSERT_FALSE(git_validate_mode("777"), "Short mode rejected");
    ASSERT_FALSE(git_validate_mode("0100644"), "Long mode rejected");
}

void test_safe_strncpy(void) {
    char dest[10];

    int ret = git_safe_strncpy(dest, "hello", sizeof(dest));
    ASSERT_EQ(ret, GIT_OK, "Copy short string OK");
    ASSERT_STR_EQ(dest, "hello", "Short string copied correctly");

    ret = git_safe_strncpy(dest, "verylongstring", sizeof(dest));
    ASSERT_EQ(ret, GIT_EBUFSIZE, "Long string returns EBUFSIZE");
    ASSERT_EQ(dest[9], '\0', "Long string is null-terminated");
}

void test_safe_path_join(void) {
    char dest[100];

    int ret = git_safe_path_join(dest, sizeof(dest), "dir", "file.txt");
    ASSERT_EQ(ret, GIT_OK, "Path join OK");
    ASSERT_STR_EQ(dest, "dir/file.txt", "Path joined correctly");

    ret = git_safe_path_join(dest, sizeof(dest), "dir/", "file.txt");
    ASSERT_EQ(ret, GIT_OK, "Path join with trailing slash OK");
    ASSERT_STR_EQ(dest, "dir/file.txt", "No double slash");

    // Test overflow
    char small[10];
    ret = git_safe_path_join(small, sizeof(small), "verylongdir", "file.txt");
    ASSERT_EQ(ret, GIT_EBUFSIZE, "Overflow detected");
}

int main(void) {
    printf("# Validation Unit Tests\n");
    TAP_PLAN(26);

    RUN_TEST(test_validate_sha1_hex_valid);
    RUN_TEST(test_validate_sha1_hex_invalid);
    RUN_TEST(test_validate_safe_path_valid);
    RUN_TEST(test_validate_safe_path_invalid);
    RUN_TEST(test_validate_mode_valid);
    RUN_TEST(test_validate_mode_invalid);
    RUN_TEST(test_safe_strncpy);
    RUN_TEST(test_safe_path_join);

    TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
