#ifndef GIT_VALIDATION_H
#define GIT_VALIDATION_H

#include "git_common.h"

// Input validation functions
int git_validate_sha1_hex(const char *sha);
int git_validate_safe_path(const char *path);
int git_validate_regular_file(const char *path);
int git_validate_mode(const char *mode);

// Safe string operations
int git_safe_strncpy(char *dest, const char *src, size_t dest_size);
int git_safe_path_join(char *dest, size_t dest_size, const char *base, const char *path);

#endif // GIT_VALIDATION_H
