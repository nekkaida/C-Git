#define _POSIX_C_SOURCE 200809L

#include "git_common.h"
#include "git_object.h"
#include "git_sha1.h"
#include "git_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

int cmd_hash_object(int argc, char *argv[]) {
    int write_object = 0;
    const char *filename = NULL;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) {
            write_object = 1;
        } else {
            filename = argv[i];
        }
    }

    if (filename == NULL) {
        fprintf(stderr, "Usage: hash-object [-w] <file>\n");
        return GIT_EINVALID;
    }

    // SECURITY: Validate that file is safe to hash
    if (!git_validate_safe_path(filename)) {
        fprintf(stderr, "Invalid or unsafe file path: %s\n", filename);
        return GIT_EINVALID;
    }

    // Open the file first (avoid TOCTOU race condition)
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return GIT_ERROR;
    }

    // Validate regular file using file descriptor (no TOCTOU)
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        fprintf(stderr, "Failed to stat file %s: %s\n", filename, strerror(errno));
        fclose(file);
        return GIT_ERROR;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Not a regular file: %s\n", filename);
        fclose(file);
        return GIT_EINVALID;
    }

    // Get file size with error checking
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to seek in file\n");
        fclose(file);
        return GIT_ERROR;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "Failed to get file size: %s\n", strerror(errno));
        fclose(file);
        return GIT_ERROR;
    }

    // Check for unreasonably large files
    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File too large: %ld bytes (max %d bytes)\n",
                file_size, MAX_FILE_SIZE);
        fclose(file);
        return GIT_EOVERFLOW;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek to beginning\n");
        fclose(file);
        return GIT_ERROR;
    }

    // Read file content
    void *content = malloc(file_size);
    if (content == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return GIT_ENOMEM;
    }

    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        fprintf(stderr, "Failed to read file %s\n", filename);
        free(content);
        return GIT_ERROR;
    }

    // Create object
    git_object *obj = git_object_new(GIT_OBJ_BLOB, file_size);
    if (!obj) {
        free(content);
        fprintf(stderr, "Failed to create object: %s\n", git_error_last());
        return GIT_ENOMEM;
    }

    memcpy(obj->data, content, file_size);
    free(content);

    // Calculate SHA-1
    int ret = git_object_hash(obj->data, obj->size, obj->type, obj->sha1);
    if (ret != GIT_OK) {
        git_object_free(obj);
        fprintf(stderr, "Failed to hash object: %s\n", git_error_last());
        return ret;
    }

    // Convert to hex
    char sha1_hex[SHA1_HEX_SIZE + 1];
    ret = git_sha1_to_hex(obj->sha1, sha1_hex, sizeof(sha1_hex));
    if (ret != GIT_OK) {
        git_object_free(obj);
        fprintf(stderr, "Failed to convert hash to hex: %s\n", git_error_last());
        return ret;
    }

    // Write object if requested
    if (write_object) {
        ret = git_object_write(obj);
        if (ret != GIT_OK) {
            git_object_free(obj);
            fprintf(stderr, "Failed to write object: %s\n", git_error_last());
            return ret;
        }
    }

    // Print hash
    printf("%s\n", sha1_hex);

    git_object_free(obj);
    return GIT_OK;
}
