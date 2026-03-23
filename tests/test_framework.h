/**
 * Simple Test Framework for x86_64 Assembler
 * Minimal testing utilities without external dependencies
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test result tracking */
typedef struct {
    int passed;
    int failed;
    int total;
    int suite_passed;
    int suite_failed;
    int suite_total;
    const char *current_suite;
    const char *current_test;
} test_state_t;

extern test_state_t g_test_state;

/* Test macros */
#define TEST_SUITE(name) \
    void test_suite_##name(void); \
    void run_test_suite_##name(void) { \
        test_suite_begin(#name); \
        test_suite_##name(); \
        test_suite_end(); \
    } \
    void test_suite_##name(void)

#define TEST(name) \
    do { \
        g_test_state.current_test = #name; \
        g_test_state.total++; \
        g_test_state.suite_total++; \
        printf("  TEST: %s ... ", #name); \
        int _test_result = test_##name(); \
        if (_test_result == 0) { \
            g_test_state.passed++; \
            g_test_state.suite_passed++; \
            printf("PASS\n"); \
        } else { \
            g_test_state.failed++; \
            g_test_state.suite_failed++; \
            printf("FAIL\n"); \
        } \
    } while (0)

/* Assertion macros */
#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            printf("\n    ASSERTION FAILED: %s\n    at %s:%d\n", \
                   #expr, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("\n    ASSERTION FAILED: Expected %lld, got %lld\n    at %s:%d\n", \
                   (long long)(expected), (long long)(actual), __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define ASSERT_EQ_HEX(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("\n    ASSERTION FAILED: Expected 0x%llX, got 0x%llX\n    at %s:%d\n", \
                   (unsigned long long)(expected), (unsigned long long)(actual), __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define ASSERT_EQ_STR(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("\n    ASSERTION FAILED: Expected \"%s\", got \"%s\"\n    at %s:%d\n", \
                   (expected), (actual), __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define ASSERT_STR_CONTAINS(haystack, needle) \
    do { \
        if ((haystack) == NULL || strstr((haystack), (needle)) == NULL) { \
            printf("\n    ASSERTION FAILED: Expected substring \"%s\" in output\n    at %s:%d\n", \
                   (needle), __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

/* Test runner helpers */
#define RUN_TEST_SUITE(name) run_test_suite_##name()

void test_init(void);
void test_suite_begin(const char *suite_name);
void test_suite_end(void);
void test_report(void);
int test_final_status(void);

/* stderr capture helpers for negative-path testing. */
int test_capture_stderr_begin(void);
char *test_capture_stderr_end(void);

#endif /* TEST_FRAMEWORK_H */
