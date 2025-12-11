/**
 * @file test_object.c
 * @brief Unit tests for Git object handling
 */

#include "test_framework.h"
#include "../include/git_object.h"
#include "../include/git_sha1.h"
#include "../include/git_common.h"
#include <string.h>

void test_object_new_blob(void) {
    git_object *obj = git_object_new(GIT_OBJ_BLOB, 100);
    ASSERT_NOT_NULL(obj, "Create blob object");
    ASSERT_EQ(obj->type, GIT_OBJ_BLOB, "Object type is blob");
    ASSERT_EQ(obj->size, 100, "Object size is correct");
    ASSERT_NOT_NULL(obj->data, "Object data allocated");
    git_object_free(obj);
}

void test_object_new_zero_size(void) {
    git_object *obj = git_object_new(GIT_OBJ_BLOB, 0);
    ASSERT_NOT_NULL(obj, "Create zero-size object");
    ASSERT_EQ(obj->size, 0, "Size is zero");
    ASSERT_NULL(obj->data, "No data allocated for zero-size");
    git_object_free(obj);
}

void test_object_new_overflow_rejected(void) {
    // Try to create object larger than MAX_OBJECT_SIZE
    git_object *obj = git_object_new(GIT_OBJ_BLOB, MAX_OBJECT_SIZE + 1);
    ASSERT_NULL(obj, "Oversized object rejected");
}

void test_object_hash_blob(void) {
    const char *content = "hello world";
    size_t content_len = strlen(content);
    unsigned char sha1[SHA1_DIGEST_SIZE];
    char hex[SHA1_HEX_SIZE + 1];

    int ret = git_object_hash(content, content_len, GIT_OBJ_BLOB, sha1);
    ASSERT_EQ(ret, GIT_OK, "Hash blob succeeds");

    git_sha1_to_hex(sha1, hex, sizeof(hex));
    // Git's blob hash of "hello world" is known
    ASSERT_STR_EQ(hex, "95d09f2b10159347eece71399a7e2e907ea3df4f",
                  "Blob hash matches Git");
}

void test_object_hash_tree(void) {
    unsigned char sha1[SHA1_DIGEST_SIZE];

    // Empty tree has a specific hash
    int ret = git_object_hash("", 0, GIT_OBJ_TREE, sha1);
    ASSERT_EQ(ret, GIT_OK, "Hash empty tree succeeds");
}

void test_object_hash_commit(void) {
    unsigned char sha1[SHA1_DIGEST_SIZE];
    const char *commit = "tree da39a3ee5e6b4b0d3255bfef95601890afd80709\n"
                         "author Test <test@example.com> 1234567890 +0000\n"
                         "committer Test <test@example.com> 1234567890 +0000\n"
                         "\n"
                         "Test commit\n";

    int ret = git_object_hash(commit, strlen(commit), GIT_OBJ_COMMIT, sha1);
    ASSERT_EQ(ret, GIT_OK, "Hash commit succeeds");
}

void test_object_hash_invalid_type(void) {
    unsigned char sha1[SHA1_DIGEST_SIZE];

    int ret = git_object_hash("data", 4, GIT_OBJ_BAD, sha1);
    ASSERT_NE(ret, GIT_OK, "Invalid type rejected");
}

void test_object_hash_null_params(void) {
    unsigned char sha1[SHA1_DIGEST_SIZE];

    int ret = git_object_hash(NULL, 10, GIT_OBJ_BLOB, sha1);
    ASSERT_NE(ret, GIT_OK, "NULL data rejected");

    ret = git_object_hash("data", 4, GIT_OBJ_BLOB, NULL);
    ASSERT_NE(ret, GIT_OK, "NULL sha1 rejected");
}

void test_object_path(void) {
    char path[100];

    int ret = git_object_path("da39a3ee5e6b4b0d3255bfef95601890afd80709",
                              path, sizeof(path));
    ASSERT_EQ(ret, GIT_OK, "Object path succeeds");
    ASSERT_STR_EQ(path, ".git/objects/da/39a3ee5e6b4b0d3255bfef95601890afd80709",
                  "Object path format correct");
}

void test_object_path_invalid(void) {
    char path[100];

    int ret = git_object_path("invalid", path, sizeof(path));
    ASSERT_NE(ret, GIT_OK, "Invalid hash rejected");

    ret = git_object_path(NULL, path, sizeof(path));
    ASSERT_NE(ret, GIT_OK, "NULL hash rejected");
}

void test_object_compress_decompress(void) {
    const char *original = "This is test data to compress and decompress.";
    size_t original_len = strlen(original);

    void *compressed = NULL;
    size_t compressed_len = 0;

    int ret = git_object_compress(original, original_len,
                                   &compressed, &compressed_len);
    ASSERT_EQ(ret, GIT_OK, "Compression succeeds");
    ASSERT_NOT_NULL(compressed, "Compressed data allocated");
    ASSERT_TRUE(compressed_len > 0, "Compressed has size");

    void *decompressed = NULL;
    size_t decompressed_len = 0;

    ret = git_object_decompress(compressed, compressed_len,
                                 &decompressed, &decompressed_len);
    ASSERT_EQ(ret, GIT_OK, "Decompression succeeds");
    ASSERT_EQ(decompressed_len, original_len, "Decompressed size matches");
    ASSERT_MEM_EQ(decompressed, original, original_len, "Data matches after roundtrip");

    free(compressed);
    free(decompressed);
}

int main(void) {
    printf("# Object Unit Tests\n");
    TAP_PLAN(21);

    RUN_TEST(test_object_new_blob);
    RUN_TEST(test_object_new_zero_size);
    RUN_TEST(test_object_new_overflow_rejected);
    RUN_TEST(test_object_hash_blob);
    RUN_TEST(test_object_hash_tree);
    RUN_TEST(test_object_hash_commit);
    RUN_TEST(test_object_hash_invalid_type);
    RUN_TEST(test_object_hash_null_params);
    RUN_TEST(test_object_path);
    RUN_TEST(test_object_path_invalid);
    RUN_TEST(test_object_compress_decompress);

    TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
