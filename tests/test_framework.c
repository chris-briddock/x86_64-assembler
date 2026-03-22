/**
 * Test Framework Implementation
 */

#include "test_framework.h"

#include <unistd.h>

/* Global test state */
test_state_t g_test_state = {0, 0, 0, NULL, NULL};

/* stderr capture state */
static int g_stderr_saved_fd = -1;
static FILE *g_stderr_capture_file = NULL;

void test_init(void) {
    g_test_state.passed = 0;
    g_test_state.failed = 0;
    g_test_state.total = 0;
    g_test_state.current_suite = NULL;
    g_test_state.current_test = NULL;
    printf("====================================\n");
    printf("     x86_64 Assembler Tests\n");
    printf("====================================\n");
}

void test_report(void) {
    printf("\n====================================\n");
    printf("           Test Results\n");
    printf("====================================\n");
    printf("Total:  %d\n", g_test_state.total);
    printf("Passed: %d\n", g_test_state.passed);
    printf("Failed: %d\n", g_test_state.failed);
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

int test_capture_stderr_begin(void) {
    if (g_stderr_capture_file != NULL || g_stderr_saved_fd != -1) {
        return -1;
    }

    g_stderr_capture_file = tmpfile();
    if (!g_stderr_capture_file) {
        return -1;
    }

    g_stderr_saved_fd = dup(fileno(stderr));
    if (g_stderr_saved_fd < 0) {
        fclose(g_stderr_capture_file);
        g_stderr_capture_file = NULL;
        return -1;
    }

    fflush(stderr);
    if (dup2(fileno(g_stderr_capture_file), fileno(stderr)) < 0) {
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
    if (dup2(g_stderr_saved_fd, fileno(stderr)) < 0) {
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
