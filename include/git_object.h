#ifndef GIT_OBJECT_H
#define GIT_OBJECT_H

#include "git_common.h"

// Object structure
typedef struct {
    git_object_t type;
    size_t size;
    unsigned char sha1[SHA1_DIGEST_SIZE];
    void *data;
} git_object;

// Object operations
git_object* git_object_new(git_object_t type, size_t size);
void git_object_free(git_object *obj);

// Read/write objects
int git_object_read(const char *sha1_hex, git_object **out);
int git_object_write(git_object *obj);

// Hash calculation
int git_object_hash(const void *data, size_t size, git_object_t type,
                    unsigned char sha1[SHA1_DIGEST_SIZE]);

// Get object path
int git_object_path(const char *sha1_hex, char *path, size_t path_size);

// Compression/decompression
int git_object_compress(const void *src, size_t src_len,
                        void **dst, size_t *dst_len);
int git_object_decompress(const void *src, size_t src_len,
                          void **dst, size_t *dst_len);

#endif // GIT_OBJECT_H
