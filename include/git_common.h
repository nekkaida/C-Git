#ifndef GIT_COMMON_H
#define GIT_COMMON_H

#include <stddef.h>
#include <stdint.h>

// Constants
#define SHA1_DIGEST_SIZE 20
#define SHA1_HEX_SIZE 40
#define SHA1_BLOCK_SIZE 64
#define GIT_MODE_DIR "40000"
#define GIT_MODE_FILE "100644"
#define GIT_MODE_EXEC "100755"
#define MAX_PATH_LEN 4096
#define MAX_CMD_LEN 8192
#define MAX_TREE_DEPTH 100
#define MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB

// Error codes
typedef enum {
    GIT_OK = 0,
    GIT_ERROR = -1,
    GIT_EINVALID = -2,
    GIT_ENOTFOUND = -3,
    GIT_EEXISTS = -4,
    GIT_EAMBIGUOUS = -5,
    GIT_EBUFSIZE = -6,
    GIT_EOVERFLOW = -7,
    GIT_ENOMEM = -8,
    GIT_EUSER = -9,
    GIT_EBAREREPO = -10,
    GIT_EUNBORNBRANCH = -11,
    GIT_EUNMERGED = -12,
    GIT_ENONFASTFORWARD = -13,
    GIT_EINVALIDSPEC = -14,
    GIT_ECONFLICT = -15,
    GIT_ELOCKED = -16,
    GIT_EMODIFIED = -17,
    GIT_EAUTH = -18,
    GIT_ECERTIFICATE = -19,
    GIT_EAPPLIED = -20,
    GIT_EPEEL = -21,
    GIT_EEOF = -22,
    GIT_EUNCOMMITTED = -23,
    GIT_EDIRECTORY = -24
} git_error_t;

// Object types
typedef enum {
    GIT_OBJ_ANY = -2,
    GIT_OBJ_BAD = -1,
    GIT_OBJ__EXT1 = 0,
    GIT_OBJ_COMMIT = 1,
    GIT_OBJ_TREE = 2,
    GIT_OBJ_BLOB = 3,
    GIT_OBJ_TAG = 4
} git_object_t;

// Error handling
const char* git_error_string(git_error_t error);
void git_error_set(git_error_t error, const char *msg);
const char* git_error_last(void);
void git_error_clear(void);

#endif // GIT_COMMON_H
