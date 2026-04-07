/**
 * Test Framework Implementation
 */

#include "test_framework.h"

#include <unistd.h>

/* Global test state */
test_state_t g_test_state = {
    .passed = 0,
    .failed = 0,
    .total = 0,
    .suite_passed = 0,
    .suite_failed = 0,
    .suite_total = 0,
    .current_suite = NULL,
    .current_test = NULL
};

/* stderr capture state */
static int g_stderr_saved_fd = -1;
static FILE *g_stderr_capture_file = NULL;

static double compute_percent(int numerator, int denominator) {
    if (denominator <= 0) {
        return 0.0;
    }
    return (100.0 * (double)numerator) / (double)denominator;
}

void test_init(void) {
    g_test_state.passed = 0;
    g_test_state.failed = 0;
    g_test_state.total = 0;
    g_test_state.suite_passed = 0;
    g_test_state.suite_failed = 0;
    g_test_state.suite_total = 0;
    g_test_state.current_suite = NULL;
    g_test_state.current_test = NULL;
    printf("====================================\n");
    printf("     x86_64 Assembler Tests\n");
    printf("====================================\n");
}

void test_suite_begin(const char *suite_name) {
    g_test_state.current_suite = suite_name;
    g_test_state.suite_passed = 0;
    g_test_state.suite_failed = 0;
    g_test_state.suite_total = 0;
    printf("\n=== Test Suite: %s ===\n", suite_name);
}

void test_suite_end(void) {
    double suite_coverage = compute_percent(g_test_state.suite_passed, g_test_state.suite_total);
    printf("  Coverage: %.1f%% (%d/%d passed)\n", suite_coverage,
           g_test_state.suite_passed, g_test_state.suite_total);
}

void test_report(void) {
    double coverage = compute_percent(g_test_state.passed, g_test_state.total);

    printf("\n====================================\n");
    printf("           Test Results\n");
    printf("====================================\n");
    printf("Total:  %d\n", g_test_state.total);
    printf("Passed: %d\n", g_test_state.passed);
    printf("Failed: %d\n", g_test_state.failed);
    printf("Coverage: %.1f%% (%d/%d passed)\n", coverage, g_test_state.passed, g_test_state.total);
    printf("====================================\n");
    
    if (g_test_state.failed == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
}

int test_final_status(void) {
    return g_test_state.failed > 0 ? 1 : 0;
}

void test_run_case(const char *test_name, int (*test_fn)(void)) {
    int test_result;

    g_test_state.current_test = test_name;
    g_test_state.total++;
    g_test_state.suite_total++;

    printf("  TEST: %s ... ", test_name);
    test_result = test_fn();

    if (test_result == 0) {
        g_test_state.passed++;
        g_test_state.suite_passed++;
        printf("PASS\n");
    } else {
        g_test_state.failed++;
        g_test_state.suite_failed++;
        printf("FAIL\n");
    }
}

int test_capture_stderr_begin(void) {
    int capture_fd;

    if (g_stderr_capture_file != NULL || g_stderr_saved_fd != -1) {
        return -1;
    }

    g_stderr_capture_file = tmpfile();
    if (!g_stderr_capture_file) {
        return -1;
    }

    g_stderr_saved_fd = dup(STDERR_FILENO);
    if (g_stderr_saved_fd < 0) {
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return -1;
    }

    capture_fd = fileno(g_stderr_capture_file);
    if (capture_fd < 0) {
        close(g_stderr_saved_fd);
        g_stderr_saved_fd = -1;
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return -1;
    }

    fflush(stderr);
    if (dup2(capture_fd, STDERR_FILENO) < 0) {
        close(g_stderr_saved_fd);
        g_stderr_saved_fd = -1;
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return -1;
    }

    return 0;
}

char *test_capture_stderr_end(void) {
    char *buf = NULL;
    long size;

    if (g_stderr_capture_file == NULL || g_stderr_saved_fd < 0) {
        return NULL;
    }

    fflush(stderr);
    if (dup2(g_stderr_saved_fd, STDERR_FILENO) < 0) {
        close(g_stderr_saved_fd);
        g_stderr_saved_fd = -1;
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return NULL;
    }
    close(g_stderr_saved_fd);
    g_stderr_saved_fd = -1;

    if (fseek(g_stderr_capture_file, 0, SEEK_END) != 0) {
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return NULL;
    }

    size = ftell(g_stderr_capture_file);
    if (size < 0 || fseek(g_stderr_capture_file, 0, SEEK_SET) != 0) {
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return NULL;
    }

    buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return NULL;
    }

    if (size > 0) {
        size_t n = fread(buf, 1, (size_t)size, g_stderr_capture_file);
        buf[n] = '\0';
    } else {
        buf[0] = '\0';
    }

    fclose(g_stderr_capture_file);
    g_stderr_capture_file = NULL;
    return buf;
}
