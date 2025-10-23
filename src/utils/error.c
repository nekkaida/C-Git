#include "git_common.h"
#include <string.h>
#include <stdio.h>

// Thread-local error state (simplified version without thread support)
static struct {
    git_error_t code;
    char message[1024];
} g_error = {GIT_OK, ""};

const char* git_error_string(git_error_t error) {
    switch (error) {
        case GIT_OK: return "No error";
        case GIT_ERROR: return "Generic error";
        case GIT_EINVALID: return "Invalid argument";
        case GIT_ENOTFOUND: return "Object not found";
        case GIT_EEXISTS: return "Object exists";
        case GIT_EAMBIGUOUS: return "Ambiguous reference";
        case GIT_EBUFSIZE: return "Buffer size insufficient";
        case GIT_EOVERFLOW: return "Integer overflow";
        case GIT_ENOMEM: return "Out of memory";
        case GIT_EUSER: return "User-generated error";
        case GIT_EBAREREPO: return "Operation not allowed on bare repository";
        case GIT_EUNBORNBRANCH: return "Unborn branch";
        case GIT_EUNMERGED: return "Unmerged entries";
        case GIT_ENONFASTFORWARD: return "Non-fast-forward";
        case GIT_EINVALIDSPEC: return "Invalid refspec";
        case GIT_ECONFLICT: return "Conflict";
        case GIT_ELOCKED: return "File locked";
        case GIT_EMODIFIED: return "File modified";
        case GIT_EAUTH: return "Authentication required";
        case GIT_ECERTIFICATE: return "Certificate error";
        case GIT_EAPPLIED: return "Patch already applied";
        case GIT_EPEEL: return "Cannot peel reference";
        case GIT_EEOF: return "Unexpected EOF";
        case GIT_EUNCOMMITTED: return "Uncommitted changes";
        case GIT_EDIRECTORY: return "Directory error";
        default: return "Unknown error";
    }
}

void git_error_set(git_error_t error, const char *msg) {
    g_error.code = error;
    if (msg) {
        strncpy(g_error.message, msg, sizeof(g_error.message) - 1);
        g_error.message[sizeof(g_error.message) - 1] = '\0';
    } else {
        g_error.message[0] = '\0';
    }
}

const char* git_error_last(void) {
    if (g_error.message[0] != '\0') {
        return g_error.message;
    }
    return git_error_string(g_error.code);
}

void git_error_clear(void) {
    g_error.code = GIT_OK;
    g_error.message[0] = '\0';
}
