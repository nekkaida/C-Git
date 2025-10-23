#ifndef GIT_TREE_H
#define GIT_TREE_H

#include "git_common.h"
#include "git_object.h"

// Tree entry structure
typedef struct {
    char mode[10];
    char name[256];
    unsigned char sha1[SHA1_DIGEST_SIZE];
} git_tree_entry;

// Tree building
typedef struct git_tree_builder git_tree_builder;

// Create tree builder
git_tree_builder* git_tree_builder_new(void);

// Add entry to tree
int git_tree_builder_add(git_tree_builder *builder, const char *mode,
                         const char *name, const unsigned char sha1[SHA1_DIGEST_SIZE]);

// Build tree object and get SHA-1
int git_tree_builder_write(git_tree_builder *builder, unsigned char sha1[SHA1_DIGEST_SIZE]);

// Free tree builder
void git_tree_builder_free(git_tree_builder *builder);

// Write tree from directory
int git_tree_write_from_directory(const char *path, unsigned char sha1[SHA1_DIGEST_SIZE]);

#endif // GIT_TREE_H
