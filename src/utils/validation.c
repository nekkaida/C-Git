#include "git_validation.h"
#include "git_common.h"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>

int git_validate_sha1_hex(const char *sha) {
    if (!sha) return 0;

    size_t len = strlen(sha);
    if (len != SHA1_HEX_SIZE) return 0;

    for (size_t i = 0; i < SHA1_HEX_SIZE; i++) {
        if (!isxdigit((unsigned char)sha[i])) return 0;
    }
    return 1;
}

int git_validate_safe_path(const char *path) {
    if (!path) return 0;

    // Check for null bytes
    if (strlen(path) != strcspn(path, "\0")) return 0;

    // Check for directory traversal patterns
    if (strstr(path, "..") != NULL) return 0;

    // No absolute paths for safety
    if (path[0] == '/' || path[0] == '\\') return 0;

    // Check length
    if (strlen(path) >= MAX_PATH_LEN) return 0;

    // Check for suspicious characters
    const char *p = path;
    while (*p) {
        if (*p == '\0' || *p == '\n' || *p == '\r') return 0;
        p++;
    }

    return 1;
}

int git_validate_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

int git_validate_mode(const char *mode) {
    if (!mode) return 0;

    // Valid Git modes: 040000 (tree), 100644 (blob), 100755 (executable), 120000 (symlink)
    if (strcmp(mode, "040000") == 0 ||  // tree
        strcmp(mode, "100644") == 0 ||  // regular file
        strcmp(mode, "100755") == 0 ||  // executable
        strcmp(mode, "120000") == 0 ||  // symlink
        strcmp(mode, "160000") == 0) {  // gitlink
        return 1;
    }

    return 0;
}

int git_safe_strncpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return GIT_EINVALID;

    size_t i;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';

    // Return error if source was truncated
    return (src[i] == '\0') ? GIT_OK : GIT_EBUFSIZE;
}

int git_safe_path_join(char *dest, size_t dest_size, const char *base, const char *path) {
    if (!dest || !base || !path || dest_size == 0) {
        return GIT_EINVALID;
    }

    size_t base_len = strlen(base);
    size_t path_len = strlen(path);

    // Check if we have enough space
    if (base_len + 1 + path_len + 1 > dest_size) {
        return GIT_EBUFSIZE;
    }

    // Copy base
    memcpy(dest, base, base_len);

    // Add separator if needed
    if (base_len > 0 && base[base_len-1] != '/' && base[base_len-1] != '\\') {
        dest[base_len] = '/';
        base_len++;
    }

    // Copy path
    memcpy(dest + base_len, path, path_len);
    dest[base_len + path_len] = '\0';

    return GIT_OK;
}
