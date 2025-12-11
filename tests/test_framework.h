/**
 * @file test_framework.h
 * @brief Lightweight unit test framework for C-Git
 *
 * Simple TAP-compatible (Test Anything Protocol) test framework.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test statistics
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

// TAP output helpers
#define TAP_PLAN(n) printf("1..%d\n", (n))

#define TEST_PASS(name) do { \
    g_tests_run++; \
    g_tests_passed++; \
    printf("ok %d - %s\n", g_tests_run, (name)); \
} while(0)

#define TEST_FAIL(name, reason) do { \
    g_tests_run++; \
    g_tests_failed++; \
    printf("not ok %d - %s\n", g_tests_run, (name)); \
    printf("# Failed: %s\n", (reason)); \
} while(0)

// Assertion macros
#define ASSERT_TRUE(expr, name) do { \
    if (expr) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, #expr " is false"); \
    } \
} while(0)

#define ASSERT_FALSE(expr, name) do { \
    if (!(expr)) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, #expr " is true"); \
    } \
} while(0)

#define ASSERT_EQ(a, b, name) do { \
    if ((a) == (b)) { \
        TEST_PASS(name); \
    } else { \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), "Expected %ld, got %ld", (long)(b), (long)(a)); \
        TEST_FAIL(name, _buf); \
    } \
} while(0)

#define ASSERT_NE(a, b, name) do { \
    if ((a) != (b)) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, "Values should not be equal"); \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b, name) do { \
    if ((a) && (b) && strcmp((a), (b)) == 0) { \
        TEST_PASS(name); \
    } else { \
        char _buf[512]; \
        snprintf(_buf, sizeof(_buf), "Expected '%s', got '%s'", \
                (b) ? (b) : "(null)", (a) ? (a) : "(null)"); \
        TEST_FAIL(name, _buf); \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr, name) do { \
    if ((ptr) != NULL) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, "Pointer is NULL"); \
    } \
} while(0)

#define ASSERT_NULL(ptr, name) do { \
    if ((ptr) == NULL) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, "Pointer is not NULL"); \
    } \
} while(0)

// Memory comparison
#define ASSERT_MEM_EQ(a, b, len, name) do { \
    if (memcmp((a), (b), (len)) == 0) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, "Memory contents differ"); \
    } \
} while(0)

// Test runner helpers
#define RUN_TEST(test_func) do { \
    printf("# Running %s\n", #test_func); \
    test_func(); \
} while(0)

#define TEST_SUMMARY() do { \
    printf("\n# Summary:\n"); \
    printf("# Tests run: %d\n", g_tests_run); \
    printf("# Passed: %d\n", g_tests_passed); \
    printf("# Failed: %d\n", g_tests_failed); \
    if (g_tests_failed == 0) { \
        printf("# All tests passed!\n"); \
    } \
} while(0)

#define TEST_EXIT_CODE() (g_tests_failed > 0 ? 1 : 0)

#endif // TEST_FRAMEWORK_H
