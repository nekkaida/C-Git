#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include <sys/stat.h>
#include <errno.h>

git_object* git_object_new(git_object_t type, size_t size) {
    git_object *obj = (git_object *)calloc(1, sizeof(git_object));
    if (!obj) {
        git_error_set(GIT_ENOMEM, "Failed to allocate object");
        return NULL;
    }

    obj->type = type;
    obj->size = size;

    if (size > 0) {
        obj->data = malloc(size);
        if (!obj->data) {
            free(obj);
            git_error_set(GIT_ENOMEM, "Failed to allocate object data");
            return NULL;
        }
    }

    return obj;
}

void git_object_free(git_object *obj) {
    if (!obj) return;
    if (obj->data) free(obj->data);
    free(obj);
}

int git_object_hash(const void *data, size_t size, git_object_t type,
                    unsigned char sha1[SHA1_DIGEST_SIZE]) {
    if (!data || !sha1) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_object_hash");
        return GIT_EINVALID;
    }

    // Get type string
    const char *type_str;
    switch (type) {
        case GIT_OBJ_BLOB: type_str = "blob"; break;
        case GIT_OBJ_TREE: type_str = "tree"; break;
        case GIT_OBJ_COMMIT: type_str = "commit"; break;
        case GIT_OBJ_TAG: type_str = "tag"; break;
        default:
            git_error_set(GIT_EINVALID, "Invalid object type");
            return GIT_EINVALID;
    }

    // Create header: "<type> <size>\0"
    char header[100];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, size);
    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        git_error_set(GIT_EBUFSIZE, "Header too large");
        return GIT_EBUFSIZE;
    }
    header[header_len] = '\0';
    header_len++;  // Include null byte

    // Calculate SHA-1
    git_sha1_ctx ctx;
    git_sha1_init(&ctx);
    git_sha1_update(&ctx, header, header_len);
    git_sha1_update(&ctx, data, size);
    git_sha1_final(sha1, &ctx);

    return GIT_OK;
}

int git_object_path(const char *sha1_hex, char *path, size_t path_size) {
    if (!sha1_hex || !path || path_size < 50) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_object_path");
        return GIT_EINVALID;
    }

    if (!git_validate_sha1_hex(sha1_hex)) {
        git_error_set(GIT_EINVALID, "Invalid SHA-1 hash");
        return GIT_EINVALID;
    }

    int len = snprintf(path, path_size, ".git/objects/%c%c/%s",
                       sha1_hex[0], sha1_hex[1], sha1_hex + 2);
    if (len < 0 || len >= (int)path_size) {
        git_error_set(GIT_EBUFSIZE, "Path buffer too small");
        return GIT_EBUFSIZE;
    }

    return GIT_OK;
}

int git_object_compress(const void *src, size_t src_len,
                        void **dst, size_t *dst_len) {
    if (!src || !dst || !dst_len) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_object_compress");
        return GIT_EINVALID;
    }

    // Allocate output buffer (compressBound gives max size needed)
    uLongf compressed_len = compressBound(src_len);
    *dst = malloc(compressed_len);
    if (!*dst) {
        git_error_set(GIT_ENOMEM, "Failed to allocate compression buffer");
        return GIT_ENOMEM;
    }

    // Compress using zlib
    int result = compress2((Bytef *)*dst, &compressed_len,
                          (const Bytef *)src, src_len,
                          Z_DEFAULT_COMPRESSION);

    if (result != Z_OK) {
        free(*dst);
        *dst = NULL;
        git_error_set(GIT_ERROR, "Compression failed");
        return GIT_ERROR;
    }

    *dst_len = compressed_len;
    return GIT_OK;
}

int git_object_decompress(const void *src, size_t src_len,
                          void **dst, size_t *dst_len) {
    if (!src || !dst || !dst_len) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_object_decompress");
        return GIT_EINVALID;
    }

    // Start with a reasonable buffer size
    size_t buffer_size = src_len * 4;  // Usually good estimate
    *dst = malloc(buffer_size);
    if (!*dst) {
        git_error_set(GIT_ENOMEM, "Failed to allocate decompression buffer");
        return GIT_ENOMEM;
    }

    // Try to decompress
    uLongf uncompressed_len = buffer_size;
    int result = uncompress((Bytef *)*dst, &uncompressed_len,
                           (const Bytef *)src, src_len);

    // If buffer was too small, try with larger buffer
    if (result == Z_BUF_ERROR) {
        free(*dst);
        buffer_size = src_len * 10;  // Try much larger
        *dst = malloc(buffer_size);
        if (!*dst) {
            git_error_set(GIT_ENOMEM, "Failed to allocate larger decompression buffer");
            return GIT_ENOMEM;
        }

        uncompressed_len = buffer_size;
        result = uncompress((Bytef *)*dst, &uncompressed_len,
                           (const Bytef *)src, src_len);
    }

    if (result != Z_OK) {
        free(*dst);
        *dst = NULL;
        git_error_set(GIT_ERROR, "Decompression failed");
        return GIT_ERROR;
    }

    *dst_len = uncompressed_len;
    return GIT_OK;
}

int git_object_write(git_object *obj) {
    if (!obj || !obj->data) {
        git_error_set(GIT_EINVALID, "Invalid object");
        return GIT_EINVALID;
    }

    // Calculate SHA-1
    int ret = git_object_hash(obj->data, obj->size, obj->type, obj->sha1);
    if (ret != GIT_OK) return ret;

    // Get hex representation
    char sha1_hex[SHA1_HEX_SIZE + 1];
    ret = git_sha1_to_hex(obj->sha1, sha1_hex, sizeof(sha1_hex));
    if (ret != GIT_OK) return ret;

    // Create directory
    char dir_path[100];
    snprintf(dir_path, sizeof(dir_path), ".git/objects/%c%c",
             sha1_hex[0], sha1_hex[1]);

    #ifdef _WIN32
    if (_mkdir(dir_path) == -1 && errno != EEXIST) {
    #else
    if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
    #endif
        char msg[200];
        snprintf(msg, sizeof(msg), "Failed to create directory %s: %s",
                 dir_path, strerror(errno));
        git_error_set(GIT_ERROR, msg);
        return GIT_ERROR;
    }

    // Build object content (header + data)
    const char *type_str;
    switch (obj->type) {
        case GIT_OBJ_BLOB: type_str = "blob"; break;
        case GIT_OBJ_TREE: type_str = "tree"; break;
        case GIT_OBJ_COMMIT: type_str = "commit"; break;
        case GIT_OBJ_TAG: type_str = "tag"; break;
        default:
            git_error_set(GIT_EINVALID, "Invalid object type");
            return GIT_EINVALID;
    }

    char header[100];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, obj->size);
    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        git_error_set(GIT_EBUFSIZE, "Header too large");
        return GIT_EBUFSIZE;
    }
    header[header_len] = '\0';
    header_len++;

    // Combine header and data - check for integer overflow
    if (obj->size > SIZE_MAX - (size_t)header_len) {
        git_error_set(GIT_EOVERFLOW, "Object size too large (would overflow)");
        return GIT_EOVERFLOW;
    }
    size_t full_size = header_len + obj->size;
    unsigned char *full_data = malloc(full_size);
    if (!full_data) {
        git_error_set(GIT_ENOMEM, "Failed to allocate object buffer");
        return GIT_ENOMEM;
    }

    memcpy(full_data, header, header_len);
    memcpy(full_data + header_len, obj->data, obj->size);

    // Compress
    void *compressed = NULL;
    size_t compressed_len = 0;
    ret = git_object_compress(full_data, full_size, &compressed, &compressed_len);
    free(full_data);

    if (ret != GIT_OK) {
        return ret;
    }

    // Write to file
    char obj_path[150];
    snprintf(obj_path, sizeof(obj_path), "%s/%s", dir_path, sha1_hex + 2);

    FILE *file = fopen(obj_path, "wb");
    if (!file) {
        free(compressed);
        char msg[200];
        snprintf(msg, sizeof(msg), "Failed to create object file %s: %s",
                 obj_path, strerror(errno));
        git_error_set(GIT_ERROR, msg);
        return GIT_ERROR;
    }

    size_t written = fwrite(compressed, 1, compressed_len, file);

    if (written != compressed_len) {
        fclose(file);
        free(compressed);
        git_error_set(GIT_ERROR, "Failed to write complete object");
        return GIT_ERROR;
    }

    if (fclose(file) != 0) {
        free(compressed);
        git_error_set(GIT_ERROR, "Failed to close object file");
        return GIT_ERROR;
    }

    free(compressed);
    return GIT_OK;
}

int git_object_read(const char *sha1_hex, git_object **out) {
    if (!sha1_hex || !out) {
        git_error_set(GIT_EINVALID, "Invalid parameters to git_object_read");
        return GIT_EINVALID;
    }

    // Get object path
    char path[MAX_PATH_LEN];
    int ret = git_object_path(sha1_hex, path, sizeof(path));
    if (ret != GIT_OK) return ret;

    // Read compressed file
    FILE *file = fopen(path, "rb");
    if (!file) {
        char msg[200];
        snprintf(msg, sizeof(msg), "Object not found: %s", sha1_hex);
        git_error_set(GIT_ENOTFOUND, msg);
        return GIT_ENOTFOUND;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to seek to end of file");
        return GIT_ERROR;
    }

    long file_size = ftell(file);

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to seek to beginning of file");
        return GIT_ERROR;
    }

    if (file_size < 0) {
        fclose(file);
        git_error_set(GIT_ERROR, "Failed to get file size");
        return GIT_ERROR;
    }

    // Read compressed data
    unsigned char *compressed = malloc(file_size);
    if (!compressed) {
        fclose(file);
        git_error_set(GIT_ENOMEM, "Failed to allocate read buffer");
        return GIT_ENOMEM;
    }

    size_t read_size = fread(compressed, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(compressed);
        git_error_set(GIT_ERROR, "Failed to read object file");
        return GIT_ERROR;
    }

    // Decompress
    void *decompressed = NULL;
    size_t decompressed_len = 0;
    ret = git_object_decompress(compressed, file_size, &decompressed, &decompressed_len);
    free(compressed);

    if (ret != GIT_OK) {
        return ret;
    }

    // Parse header
    unsigned char *data = (unsigned char *)decompressed;
    unsigned char *null_pos = memchr(data, '\0', decompressed_len);
    if (!null_pos) {
        free(decompressed);
        git_error_set(GIT_ERROR, "Invalid object format: no null byte in header");
        return GIT_ERROR;
    }

    size_t header_len = null_pos - data + 1;
    char *header = (char *)data;

    // Parse type and size
    char type_str[32];
    size_t obj_size;
    if (sscanf(header, "%31s %zu", type_str, &obj_size) != 2) {
        free(decompressed);
        git_error_set(GIT_ERROR, "Invalid object header format");
        return GIT_ERROR;
    }

    // Determine type
    git_object_t type;
    if (strcmp(type_str, "blob") == 0) type = GIT_OBJ_BLOB;
    else if (strcmp(type_str, "tree") == 0) type = GIT_OBJ_TREE;
    else if (strcmp(type_str, "commit") == 0) type = GIT_OBJ_COMMIT;
    else if (strcmp(type_str, "tag") == 0) type = GIT_OBJ_TAG;
    else {
        free(decompressed);
        char msg[100];
        snprintf(msg, sizeof(msg), "Unknown object type: %s", type_str);
        git_error_set(GIT_ERROR, msg);
        return GIT_ERROR;
    }

    // Create object
    git_object *obj = git_object_new(type, obj_size);
    if (!obj) {
        free(decompressed);
        return GIT_ENOMEM;
    }

    // Copy data
    memcpy(obj->data, data + header_len, obj_size);

    // Set SHA-1
    ret = git_sha1_from_hex(sha1_hex, obj->sha1);
    if (ret != GIT_OK) {
        git_object_free(obj);
        free(decompressed);
        return ret;
    }

    free(decompressed);
    *out = obj;
    return GIT_OK;
}
