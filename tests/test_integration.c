/**
 * Integration Tests
 * End-to-end testing of the assembler pipeline
 */

#include "test_framework.h"
#include "x86_64_asm/x86_64_asm.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>

typedef struct {
    int line_number;
    uint64_t address;
    uint8_t bytes[16];
    int byte_count;
    char raw_line[512];
} listing_row_t;

typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} test_elf64_ehdr_t;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} test_elf64_shdr_t;

typedef struct {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} test_elf64_sym_t;

static size_t decode_uleb128_at(const unsigned char *buf, size_t len, size_t pos, uint64_t *out) {
    uint64_t value = 0;
    int shift = 0;

    while (pos < len && shift < 64) {
        uint8_t byte = buf[pos++];
        value |= ((uint64_t)(byte & 0x7F)) << shift;
        if ((byte & 0x80) == 0) {
            *out = value;
            return pos;
        }
        shift += 7;
    }

    return (size_t)-1;
}

static size_t decode_sleb128_at(const unsigned char *buf, size_t len, size_t pos, int64_t *out) {
    int64_t value = 0;
    int shift = 0;
    uint8_t byte = 0;

    while (pos < len && shift < 64) {
        byte = buf[pos++];
        value |= ((int64_t)(byte & 0x7F)) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    if ((byte & 0x80) != 0) return (size_t)-1;

    if (shift < 64 && (byte & 0x40)) {
        value |= -((int64_t)1 << shift);
    }

    *out = value;
    return pos;
}

/* Assemble from an existing file path and run resulting executable. */
static int assemble_file_and_run(const char *asm_file) {
    char bin_file[] = "/tmp/test_XXXXXX";

    /* Generate binary name from asm name */
    strncpy(bin_file, asm_file, sizeof(bin_file)-1);
    bin_file[sizeof(bin_file)-1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    /* Initialize assembler */
    assembler_context_t *ctx = asm_init();
    if (!ctx) {
        return -1;
    }

    /* Assemble the file */
    int result = asm_assemble_file(ctx, asm_file);
    if (result < 0) {
        asm_free(ctx);
        return -1;
    }

    /* Write ELF output */
    result = asm_write_elf64(ctx, bin_file);
    asm_free(ctx);

    if (result < 0) {
        return -1;
    }

    /* Make executable and run */
    chmod(bin_file, 0755);

    pid_t pid = fork();
    if (pid < 0) {
        unlink(bin_file);
        return -1;
    }

    if (pid == 0) {
        /* Child - run the binary */
        execl(bin_file, bin_file, NULL);
        _exit(127);
    }

    /* Parent - wait for child */
    int status;
    waitpid(pid, &status, 0);
    unlink(bin_file);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

/* Test helper: Assemble source and run it, return exit code */
static int assemble_and_run(const char *source) {
    /* Create temp files */
    char asm_file[] = "/tmp/test_XXXXXX";
    
    int fd = mkstemp(asm_file);
    if (fd < 0) return -1;
    
    /* Write assembly to temp file */
    write(fd, source, strlen(source));
    close(fd);
    
    int result = assemble_file_and_run(asm_file);
    unlink(asm_file);
    return result;
}

/* Assemble a fixture file and ensure the emitted ELF artifact is non-empty. */
static int assemble_fixture_and_verify_nonempty_output(const char *fixture_path) {
    assembler_context_t *ctx;
    char out_file[] = "/tmp/test_stress_out_XXXXXX";
    struct stat st;
    int out_fd;
    int result;

    if (!fixture_path) {
        return -1;
    }

    out_fd = mkstemp(out_file);
    if (out_fd < 0) {
        return -1;
    }
    close(out_fd);

    ctx = asm_init();
    if (!ctx) {
        unlink(out_file);
        return -1;
    }

    result = asm_assemble_file(ctx, fixture_path);
    if (result == 0) {
        result = asm_write_elf64(ctx, out_file);
    }

    asm_free(ctx);

    if (result != 0) {
        unlink(out_file);
        return -1;
    }

    if (stat(out_file, &st) != 0 || st.st_size <= 0) {
        unlink(out_file);
        return -1;
    }

    unlink(out_file);
    return 0;
}

static char *disassemble_buffer_to_string(const uint8_t *code, size_t size, uint64_t base) {
    FILE *fp;
    long length;
    char *buffer;

    fp = tmpfile();
    if (!fp) {
        return NULL;
    }

    if (disassemble_code_buffer(code, size, base, fp) < 0) {
        fclose(fp);
        return NULL;
    }

    if (fflush(fp) != 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    length = ftell(fp);
    if (length < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    buffer = (char *)malloc((size_t)length + 1U);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1U, (size_t)length, fp) != (size_t)length) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(fp);
    return buffer;
}

static char *capture_command_output(const char *cmd) {
    FILE *fp;
    char line[512];
    char *buffer;
    size_t len = 0;
    size_t cap = 4096;
    int status;

    if (!cmd) {
        return NULL;
    }

    fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    buffer = (char *)malloc(cap);
    if (!buffer) {
        pclose(fp);
        return NULL;
    }
    buffer[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        size_t chunk = strlen(line);
        if (len + chunk + 1U > cap) {
            size_t new_cap = cap * 2U;
            while (len + chunk + 1U > new_cap) {
                new_cap *= 2U;
            }
            char *tmp = (char *)realloc(buffer, new_cap);
            if (!tmp) {
                free(buffer);
                pclose(fp);
                return NULL;
            }
            buffer = tmp;
            cap = new_cap;
        }
        memcpy(buffer + len, line, chunk);
        len += chunk;
        buffer[len] = '\0';
    }

    status = pclose(fp);
    if (status != 0 && len == 0U) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

static const symbol_t *find_symbol_by_name(assembler_context_t *ctx, const char *name) {
    for (int i = 0; i < ctx->symbol_count; i++) {
        if (strcmp(ctx->symbols[i].name, name) == 0) {
            return &ctx->symbols[i];
        }
    }
    return NULL;
}

static int parse_listing_row(const char *line, listing_row_t *out) {
    char local[512];
    uint8_t parsed_bytes[16];
    char *p;
    char *end;
    int count = 0;

    if (!line || !out) {
        return 0;
    }

    strncpy(local, line, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';
    p = local;

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    if (!isdigit((unsigned char)*p)) {
        return 0;
    }

    long ln = strtol(p, &end, 10);
    if (end == p) {
        return 0;
    }
    p = end;

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    unsigned long long addr = strtoull(p, &end, 16);
    if (end == p) {
        return 0;
    }
    p = end;

    while (*p && count < 16) {
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }

        if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])) {
            break;
        }
        if (isxdigit((unsigned char)p[2])) {
            break;
        }

        char hex[3];
        hex[0] = p[0];
        hex[1] = p[1];
        hex[2] = '\0';
        parsed_bytes[count++] = (uint8_t)strtoul(hex, NULL, 16);
        p += 2;
    }

    if (count <= 0) {
        return 0;
    }

    memset(out, 0, sizeof(*out));
    out->line_number = (int)ln;
    out->address = (uint64_t)addr;
    out->byte_count = count;
    memcpy(out->bytes, parsed_bytes, (size_t)count);
    strncpy(out->raw_line, line, sizeof(out->raw_line) - 1);
    out->raw_line[sizeof(out->raw_line) - 1] = '\0';
    return 1;
}

static int assemble_and_write_listing(const char *source,
                                      char *asm_file,
                                      size_t asm_file_size,
                                      char *out_file,
                                      size_t out_file_size,
                                      char *lst_file,
                                      size_t lst_file_size,
                                      assembler_context_t **out_ctx) {
    int fd = -1;
    assembler_context_t *ctx = NULL;

    if (!source || !asm_file || !out_file || !lst_file || !out_ctx) {
        return -1;
    }

    *out_ctx = NULL;
    snprintf(asm_file, asm_file_size, "/tmp/test_listing_src_XXXXXX");
    fd = mkstemp(asm_file);
    if (fd < 0) {
        return -1;
    }

    if (write(fd, source, strlen(source)) != (ssize_t)strlen(source)) {
        close(fd);
        unlink(asm_file);
        return -1;
    }
    close(fd);

    snprintf(out_file, out_file_size, "%s.out", asm_file);
    snprintf(lst_file, lst_file_size, "%s.lst", out_file);

    ctx = asm_init();
    if (!ctx) {
        unlink(asm_file);
        return -1;
    }
    ctx->emit_listing = true;

    if (asm_assemble_file(ctx, asm_file) < 0) {
        asm_free(ctx);
        unlink(asm_file);
        return -1;
    }

    if (asm_write_elf64(ctx, out_file) < 0) {
        asm_free(ctx);
        unlink(asm_file);
        unlink(out_file);
        unlink(lst_file);
        return -1;
    }

    if (asm_write_listing(ctx, out_file) < 0) {
        asm_free(ctx);
        unlink(asm_file);
        unlink(out_file);
        unlink(lst_file);
        return -1;
    }

    *out_ctx = ctx;
    return 0;
}

/* Test: Simple exit program */
static int test_integration_exit(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $42\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(42, exit_code);
    return 0;
}

/* Test: Arithmetic operations */
static int test_integration_arithmetic(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $10\n"
        "    add rax, $5\n"      /* rax = 15 */
        "    sub rax, $3\n"    /* rax = 12 */
        "    mov rdi, rax\n"
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(12, exit_code);
    return 0;
}

/* Test: Register moves */
static int test_integration_registers(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $100\n"
        "    mov rbx, rax\n"   /* rbx = 100 */
        "    mov rdi, rbx\n"
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(100, exit_code);
    return 0;
}

/* Test: Conditional jump */
static int test_integration_conditional(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $5\n"
        "    cmp rax, $5\n"
        "    je equal\n"
        "    mov rdi, $1\n"    /* Exit 1 if not equal */
        "    jmp exit\n"
        "equal:\n"
        "    mov rdi, $0\n"    /* Exit 0 if equal */
        "exit:\n"
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(0, exit_code);
    return 0;
}

/* Test: Loop */
static int test_integration_loop(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rcx, $5\n"    /* Loop counter */
        "    mov rax, $0\n"    /* Sum */
        "myloop:\n"
        "    add rax, $1\n"
        "    dec rcx\n"
        "    jne myloop\n"     /* Loop 5 times (jne = jnz) */
        "    mov rdi, rax\n"   /* Exit with sum = 5 */
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(5, exit_code);
    return 0;
}

/* Test: Local labels should be scoped to the nearest non-local label. */
static int test_integration_local_labels_scoped(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    jmp .done\n"
        ".fail:\n"
        "    mov rdi, $99\n"
        "    jmp .exit\n"
        ".done:\n"
        "    mov rdi, $21\n"
        ".exit:\n"
        "    mov rax, $60\n"
        "    syscall\n"
        "helper:\n"
        "    jmp .done\n"
        ".fail:\n"
        "    mov rax, $1\n"
        ".done:\n"
        "    ret\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(21, exit_code);
    return 0;
}

/* Test: Anonymous labels should resolve with @B/@F references. */
static int test_integration_anonymous_labels(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rcx, $2\n"
        "@@:\n"
        "    dec rcx\n"
        "    jnz @B\n"
        "    jmp @F\n"
        "    mov rdi, $1\n"
        "@@:\n"
        "    mov rdi, $7\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(7, exit_code);
    return 0;
}

/* Test: Push and pop */
static int test_integration_pushpop(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $0x42\n"
        "    push rax\n"
        "    mov rax, $0\n"
        "    pop rax\n"
        "    mov rdi, rax\n"
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(0x42, exit_code);
    return 0;
}

/* Test: Call and ret */
static int test_integration_callret(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rdi, $99\n"
        "    call double\n"
        "    mov rdi, rax\n"
        "    mov rax, $60\n"
        "    syscall\n"
        "double:\n"
        "    mov rax, rdi\n"
        "    add rax, rdi\n"   /* Return rdi * 2 = 198 */
        "    ret\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(198, exit_code);
    return 0;
}

/* Test: enter/leave and legacy sign-extension helpers should assemble and execute */
static int test_integration_enter_legacy_signext(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    enter $0, $0\n"
        "    mov al, $0x80\n"
        "    cbw\n"
        "    cwde\n"
        "    cdqe\n"
        "    cwd\n"
        "    cdq\n"
        "    cqo\n"
        "    leave\n"
        "    mov rax, $60\n"
        "    mov rdi, $23\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(23, exit_code);
    return 0;
}

/* Test: 8-bit register operations */
static int test_integration_8bit(void) {
    /* Test 8-bit low registers (al, bl, cl, dl) which are more commonly used
     * Note: 8-bit high registers (ah, bh, ch, dh) have encoding limitations
     * in 64-bit mode with REX prefixes */
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $0\n"
        "    mov al, $0x42\n"   /* Set AL to 0x42 */
        "    mov bl, $0x10\n"   /* Set BL to 0x10 */
        "    add al, bl\n"      /* AL = 0x52 */
        "    movzx rdi, al\n"   /* Zero-extend to RDI */
        "    mov rax, $60\n"
        "    syscall\n";
    
    int exit_code = assemble_and_run(source);
    ASSERT_EQ(0x52, exit_code);  /* 0x42 + 0x10 = 0x52 = 82 */
    return 0;
}

/* Test: Data section usage */
static int test_integration_data(void) {
    const char *source = 
        "section .data\n"
        "value: dq $72\n"
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rdi, [value]\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(72, exit_code);
    return 0;
}

/* Test: Named section variants should map to data/text behavior */
static int test_integration_named_sections(void) {
    const char *source =
        "section .data.custom\n"
        "value: dq $91\n"
        "section .text.hot\n"
        "global _start\n"
        "_start:\n"
        "    mov rdi, [value]\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(91, exit_code);
    return 0;
}

/* Test: segment alias should behave the same as section */
static int test_integration_segment_alias(void) {
    const char *source =
        "segment .data\n"
        "value: dq $66\n"
        "segment .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rdi, [value]\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(66, exit_code);
    return 0;
}

/* Test: Preprocessor directives should participate in assembly flow */
static int test_integration_preprocessor_conditionals(void) {
    const char *source =
        "%define EXIT_CODE 23\n"
        "%if EXIT_CODE\n"
        "%warning selecting defined exit path\n"
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, EXIT_CODE\n"
        "    syscall\n"
        "%else\n"
        "%error unexpected inactive branch\n"
        "%endif\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(23, exit_code);
    return 0;
}

/* Test: Empty program should fail gracefully */
static int test_integration_empty(void) {
    const char *source = "";
    char *captured_err;
    
    /* Try to assemble empty source */
    assembler_context_t *ctx = asm_init();
    if (!ctx) return 1;

    ASSERT_EQ(0, test_capture_stderr_begin());
    
    int result = asm_assemble(ctx, source);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    /* Empty source should assemble as a no-op without parser/encoder errors. */
    ASSERT_EQ(0, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_EQ_STR("", captured_err);

    free(captured_err);
    return 0;
}

/* Test: Invalid instruction should fail */
static int test_integration_invalid(void) {
    const char *source = 
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    invalid_instruction\n";
    char *captured_err;
    
    assembler_context_t *ctx = asm_init();
    if (!ctx) return 1;

    ASSERT_EQ(0, test_capture_stderr_begin());
    
    int result = asm_assemble(ctx, source);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);
    
    /* Should fail with invalid instruction */
    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Unknown instruction");
    ASSERT_STR_CONTAINS(captured_err, "Parsing failed");

    free(captured_err);
    return 0;
}

/* Test: Invalid ENTER operand range should fail with clear diagnostic */
static int test_integration_enter_invalid_operands(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    enter $70000, $0\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n";
    char *captured_err;

    assembler_context_t *ctx = asm_init();
    if (!ctx) return 1;

    ASSERT_EQ(0, test_capture_stderr_begin());

    int result = asm_assemble(ctx, source);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line");
    ASSERT_STR_CONTAINS(captured_err, "column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "enter first operand must fit in 16 bits");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: Invalid ENTER nesting-level operand should fail with clear diagnostic */
static int test_integration_enter_invalid_nesting(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    enter $0, $300\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n";
    char *captured_err;

    assembler_context_t *ctx = asm_init();
    if (!ctx) return 1;

    ASSERT_EQ(0, test_capture_stderr_begin());

    int result = asm_assemble(ctx, source);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line");
    ASSERT_STR_CONTAINS(captured_err, "column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "enter second operand must fit in 8 bits");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: integration failure matrix should emit actionable diagnostics across parser/encoder paths */
static int test_integration_failure_scenarios_matrix(void) {
    struct {
        const char *name;
        const char *source;
        const char *must_contain_1;
        const char *must_contain_2;
    } cases[] = {
        {
            "unknown instruction typo",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    ad rax, rbx\n",
            "Unknown instruction 'ad'",
            "Did you mean 'add'"
        },
        {
            "invalid memory scale",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    mov rax, [rbx + rcx * 5]\n",
            "Expected scale factor (1, 2, 4, or 8)",
            "Suggestion:"
        },
        {
            "rip relative with index",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    mov rax, [rip + rcx * 2]\n",
            "RIP-relative addressing cannot combine base/index registers",
            "Suggestion:"
        },
        {
            "segment override non-memory",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    mov rax, fs:rax\n",
            "Segment override requires a memory operand",
            "Suggestion:"
        },
        {
            "sse move mem dst imm src",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    movups [rax], $1\n",
            "memory destination requires XMM source",
            "Suggestion:"
        },
        {
            "sse arithmetic dst non xmm",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    addss rax, xmm1\n",
            "destination must be XMM register",
            "Suggestion:"
        },
        {
            "enter size out of range",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    enter $70000, $0\n",
            "enter first operand must fit in 16 bits",
            "Suggestion:"
        },
        {
            "enter nesting out of range",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    enter $0, $300\n",
            "enter second operand must fit in 8 bits",
            "Suggestion:"
        },
        {
            "xchg high8 rex conflict",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    xchg ah, r8b\n",
            "Cannot use AH/BH/CH/DH",
            "Suggestion:"
        },
        {
            "test high8 rex conflict",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    test ah, r8b\n",
            "Cannot use AH/BH/CH/DH",
            "Suggestion:"
        },
        {
            "undefined label did you mean",
            "section .text\n"
            "global _start\n"
            "_start:\n"
            "    jmp donee\n"
            "done:\n"
            "    mov rax, $60\n"
            "    mov rdi, $0\n"
            "    syscall\n",
            "Undefined symbol 'donee'",
            "Did you mean 'done'"
        }
    };

    size_t case_count = sizeof(cases) / sizeof(cases[0]);
    for (size_t i = 0; i < case_count; i++) {
        assembler_context_t *ctx = asm_init();
        char *captured_err;
        int result;

        ASSERT_NOT_NULL(ctx);
        ASSERT_EQ(0, test_capture_stderr_begin());
        result = asm_assemble(ctx, cases[i].source);
        captured_err = test_capture_stderr_end();
        asm_free(ctx);

        ASSERT_EQ(-1, result);
        ASSERT_NOT_NULL(captured_err);
        ASSERT_STR_CONTAINS(captured_err, cases[i].must_contain_1);
        ASSERT_STR_CONTAINS(captured_err, cases[i].must_contain_2);
        ASSERT_STR_CONTAINS(captured_err, "Error at line");

        free(captured_err);
    }

    return 0;
}

/* Regression test: sectionless source should still enter at _start */
static int test_integration_sectionless_entrypoint(void) {
    const char *source =
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $77\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(77, exit_code);
    return 0;
}

/* Test: Relative include file should be resolved from source file directory */
static int test_integration_include_relative(void) {
    char dir_template[] = "/tmp/asm_inc_XXXXXX";
    char main_path[512];
    char include_path[512];
    FILE *main_f;
    FILE *inc_f;
    int exit_code;

    char *dir = mkdtemp(dir_template);
    ASSERT_NOT_NULL(dir);

    ASSERT_TRUE(snprintf(main_path, sizeof(main_path), "%s/main.asm", dir) < (int)sizeof(main_path));
    ASSERT_TRUE(snprintf(include_path, sizeof(include_path), "%s/common.inc", dir) < (int)sizeof(include_path));

    inc_f = fopen(include_path, "w");
    ASSERT_NOT_NULL(inc_f);
    fprintf(inc_f,
            "helper:\n"
            "    mov rax, $60\n"
            "    mov rdi, $33\n"
            "    syscall\n");
    fclose(inc_f);

    main_f = fopen(main_path, "w");
    ASSERT_NOT_NULL(main_f);
    fprintf(main_f,
            "section .text\n"
            "global _start\n"
            ".include \"common.inc\"\n"
            "_start:\n"
            "    jmp helper\n");
    fclose(main_f);

    exit_code = assemble_file_and_run(main_path);

    unlink(main_path);
    unlink(include_path);
    rmdir(dir);

    ASSERT_EQ(33, exit_code);
    return 0;
}

/* Test: Circular includes should fail with clear diagnostics */
static int test_integration_include_circular_fails(void) {
    char dir_template[] = "/tmp/asm_circular_inc_XXXXXX";
    char main_path[512];
    char a_path[512];
    char b_path[512];
    FILE *main_f;
    FILE *a_f;
    FILE *b_f;
    assembler_context_t *ctx;
    char *captured_err;
    int result;

    char *dir = mkdtemp(dir_template);
    ASSERT_NOT_NULL(dir);

    ASSERT_TRUE(snprintf(main_path, sizeof(main_path), "%s/main.asm", dir) < (int)sizeof(main_path));
    ASSERT_TRUE(snprintf(a_path, sizeof(a_path), "%s/a.inc", dir) < (int)sizeof(a_path));
    ASSERT_TRUE(snprintf(b_path, sizeof(b_path), "%s/b.inc", dir) < (int)sizeof(b_path));

    a_f = fopen(a_path, "w");
    ASSERT_NOT_NULL(a_f);
    fprintf(a_f, ".include \"b.inc\"\n");
    fclose(a_f);

    b_f = fopen(b_path, "w");
    ASSERT_NOT_NULL(b_f);
    fprintf(b_f, ".include \"a.inc\"\n");
    fclose(b_f);

    main_f = fopen(main_path, "w");
    ASSERT_NOT_NULL(main_f);
    fprintf(main_f,
            "section .text\n"
            "global _start\n"
            ".include \"a.inc\"\n"
            "_start:\n"
            "    mov rax, $60\n"
            "    xor rdi, rdi\n"
            "    syscall\n");
    fclose(main_f);

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(0, test_capture_stderr_begin());
    result = asm_assemble_file(ctx, main_path);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    unlink(main_path);
    unlink(a_path);
    unlink(b_path);
    rmdir(dir);

    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Circular include detected");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: Duplicate include should assemble deterministically without crashing */
static int test_integration_include_duplicate_symbol_handled(void) {
    char dir_template[] = "/tmp/asm_dup_inc_XXXXXX";
    char main_path[512];
    char common_path[512];
    FILE *main_f;
    FILE *common_f;
    assembler_context_t *ctx;
    char *captured_err;
    int result;
    int exit_code;

    char *dir = mkdtemp(dir_template);
    ASSERT_NOT_NULL(dir);

    ASSERT_TRUE(snprintf(main_path, sizeof(main_path), "%s/main.asm", dir) < (int)sizeof(main_path));
    ASSERT_TRUE(snprintf(common_path, sizeof(common_path), "%s/common.inc", dir) < (int)sizeof(common_path));

    common_f = fopen(common_path, "w");
    ASSERT_NOT_NULL(common_f);
        fprintf(common_f, "helper:\n    nop\n");
    fclose(common_f);

    main_f = fopen(main_path, "w");
    ASSERT_NOT_NULL(main_f);
    fprintf(main_f,
            "section .text\n"
            "global _start\n"
            ".include \"common.inc\"\n"
            ".include \"common.inc\"\n"
            "_start:\n"
                "    mov rax, $60\n"
                "    xor rdi, rdi\n"
                "    syscall\n");
    fclose(main_f);

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(0, test_capture_stderr_begin());
    result = asm_assemble_file(ctx, main_path);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    exit_code = assemble_file_and_run(main_path);

    unlink(main_path);
    unlink(common_path);
    rmdir(dir);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0, exit_code);
    ASSERT_NOT_NULL(captured_err);
    if (captured_err[0] != '\0') {
        ASSERT_STR_CONTAINS(captured_err, "Redefined");
        ASSERT_STR_CONTAINS(captured_err, "Suggestion:");
    }

    free(captured_err);
    return 0;
}

/* Test: Include nesting beyond MAX_INCLUDE_DEPTH should fail with actionable diagnostics */
static int test_integration_include_depth_overflow_fails(void) {
    char dir_template[] = "/tmp/asm_inc_depth_XXXXXX";
    char main_path[512];
    char paths[12][512];
    FILE *f;
    assembler_context_t *ctx;
    char *captured_err;
    int result;

    char *dir = mkdtemp(dir_template);
    ASSERT_NOT_NULL(dir);

    ASSERT_TRUE(snprintf(main_path, sizeof(main_path), "%s/main.asm", dir) < (int)sizeof(main_path));
    for (int i = 0; i < 12; i++) {
        ASSERT_TRUE(snprintf(paths[i], sizeof(paths[i]), "%s/inc%02d.inc", dir, i + 1) < (int)sizeof(paths[i]));
    }

    for (int i = 0; i < 12; i++) {
        f = fopen(paths[i], "w");
        ASSERT_NOT_NULL(f);
        if (i < 11) {
            fprintf(f, ".include \"inc%02d.inc\"\n", i + 2);
        } else {
            fprintf(f, "done_label:\n    nop\n");
        }
        fclose(f);
    }

    f = fopen(main_path, "w");
    ASSERT_NOT_NULL(f);
    fprintf(f,
            "section .text\n"
            "global _start\n"
            ".include \"inc01.inc\"\n"
            "_start:\n"
            "    mov rax, $60\n"
            "    xor rdi, rdi\n"
            "    syscall\n");
    fclose(f);

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(0, test_capture_stderr_begin());
    result = asm_assemble_file(ctx, main_path);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    unlink(main_path);
    for (int i = 0; i < 12; i++) {
        unlink(paths[i]);
    }
    rmdir(dir);

    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Include depth exceeded");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: SSE instructions should assemble and execute in a simple program */
static int test_integration_sse_smoke(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $16\n"
        "    mov rax, $0x3f800000\n"
        "    mov [rsp], rax\n"
        "    movss xmm0, [rsp]\n"
        "    addss xmm0, xmm0\n"
        "    add rsp, $16\n"
        "    mov rax, $60\n"
        "    mov rdi, $7\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(7, exit_code);
    return 0;
}

/* Test: Mix SSE operations with integer loop/control-flow paths */
static int test_integration_sse_mixed_controlflow(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $16\n"
        "    mov rax, $0x3f800000\n"   /* 1.0f bit pattern */
        "    mov [rsp], rax\n"
        "    mov rcx, $3\n"
        "    mov rbx, $0\n"
        "loop_start:\n"
        "    movss xmm0, [rsp]\n"
        "    addss xmm0, xmm0\n"
        "    add rbx, $3\n"
        "    dec rcx\n"
        "    jne loop_start\n"
        "    add rsp, $16\n"
        "    mov rdi, rbx\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(9, exit_code);
    return 0;
}

/* Test: Scalar SSE result should round-trip back into scalar register path */
static int test_integration_sse_scalar_to_gpr_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $32\n"
        "    mov rax, $0x3f800000\n"   /* 1.0f */
        "    mov [rsp], rax\n"
        "    movss xmm0, [rsp]\n"
        "    addss xmm0, xmm0\n"      /* 2.0f -> 0x40000000 */
        "    movss [rsp + 8], xmm0\n"
        "    mov eax, [rsp + 8]\n"
        "    cmp eax, $0x40000000\n"
        "    jne fail\n"
        "    mov rdi, $11\n"
        "    jmp done\n"
        "fail:\n"
        "    mov rdi, $77\n"
        "done:\n"
        "    add rsp, $32\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(11, exit_code);
    return 0;
}

/* Test: Program should touch all 16 XMM registers and execute deterministically */
static int test_integration_sse_all_xmm_registers_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $32\n"
        "    pxor xmm0, xmm0\n"
        "    pxor xmm1, xmm1\n"
        "    pxor xmm2, xmm2\n"
        "    pxor xmm3, xmm3\n"
        "    pxor xmm4, xmm4\n"
        "    pxor xmm5, xmm5\n"
        "    pxor xmm6, xmm6\n"
        "    pxor xmm7, xmm7\n"
        "    pxor xmm8, xmm8\n"
        "    pxor xmm9, xmm9\n"
        "    pxor xmm10, xmm10\n"
        "    pxor xmm11, xmm11\n"
        "    pxor xmm12, xmm12\n"
        "    pxor xmm13, xmm13\n"
        "    pxor xmm14, xmm14\n"
        "    pcmpeqd xmm15, xmm15\n"
        "    movdqu [rsp], xmm15\n"
        "    mov rdi, [rsp]\n"
        "    add rsp, $32\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(255, exit_code);
    return 0;
}

/* Test: Label subtraction in equ should resolve to absolute immediate constant. */
static int test_integration_label_arithmetic_equ(void) {
    const char *source =
        "section .text\n"
        "start:\n"
        "    nop\n"
        "end:\n"
        "DELTA equ end - start\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, DELTA\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(1, exit_code);
    return 0;
}

/* Test: weak/hidden directives should mark symbol attributes in the table. */
static int test_integration_weak_hidden_attributes(void) {
    const char *source =
        "section .text\n"
        "bar:\n"
        "    nop\n"
        "hidden bar\n"
        "weak foo\n"
        "foo:\n"
        "    nop\n"
        ".hidden foo\n";

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ASSERT_EQ(0, asm_assemble(ctx, source));

    const symbol_t *foo = find_symbol_by_name(ctx, "foo");
    const symbol_t *bar = find_symbol_by_name(ctx, "bar");
    ASSERT_NOT_NULL(foo);
    ASSERT_NOT_NULL(bar);

    ASSERT_TRUE(foo->is_weak);
    ASSERT_TRUE(foo->is_hidden);
    ASSERT_FALSE(bar->is_weak);
    ASSERT_TRUE(bar->is_hidden);

    asm_free(ctx);
    return 0;
}

/* Test: Multiple SSE instructions in sequence should execute and produce stable result */
static int test_integration_sse_sequence_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $64\n"
        "    mov rax, $3\n"
        "    mov [rsp], rax\n"
        "    mov rax, $4\n"
        "    mov [rsp + 8], rax\n"
        "    mov rax, $7\n"
        "    mov [rsp + 16], rax\n"
        "    mov rax, $9\n"
        "    mov [rsp + 24], rax\n"
        "    movdqu xmm0, [rsp]\n"
        "    movdqu xmm1, [rsp + 16]\n"
        "    paddd xmm0, xmm1\n"
        "    psubd xmm0, [rsp + 16]\n"
        "    pxor xmm2, xmm2\n"
        "    por xmm2, xmm0\n"
        "    movdqu [rsp + 32], xmm2\n"
        "    mov rdi, [rsp + 32]\n"
        "    add rsp, $64\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(3, exit_code);
    return 0;
}

/* Test: SSE helper call should preserve caller XMM state via explicit save/restore */
static int test_integration_sse_function_call_preserve_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $96\n"
        "    pxor xmm1, xmm1\n"
        "    pcmpeqd xmm1, xmm1\n"   /* Caller sentinel */
        "    call sse_worker\n"
        "    movdqu [rsp + 48], xmm1\n"
        "    mov rax, [rsp + 48]\n"
        "    add rax, $1\n"
        "    jne bad\n"
        "    mov rdi, $13\n"
        "    jmp done\n"
        "bad:\n"
        "    mov rdi, $99\n"
        "done:\n"
        "    add rsp, $96\n"
        "    mov rax, $60\n"
        "    syscall\n"
        "\n"
        "sse_worker:\n"
        "    movdqu [rsp + 16], xmm1\n"  /* Save caller XMM1 */
        "    pxor xmm0, xmm0\n"
        "    pcmpeqd xmm0, xmm0\n"
        "    movdqu xmm1, [rsp + 16]\n"  /* Restore caller XMM1 */
        "    ret\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(13, exit_code);
    return 0;
}

/* Test: Packed SSE arithmetic should produce deterministic memory result */
static int test_integration_sse_packed_arith_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $64\n"
        "    mov rax, $1\n"
        "    mov [rsp], rax\n"
        "    mov rax, $2\n"
        "    mov [rsp + 8], rax\n"
        "    mov rax, $4\n"
        "    mov [rsp + 16], rax\n"
        "    mov rax, $8\n"
        "    mov [rsp + 24], rax\n"
        "    movdqu xmm0, [rsp]\n"
        "    movdqu xmm1, [rsp + 16]\n"
        "    paddd xmm0, xmm1\n"
        "    movdqu [rsp + 32], xmm0\n"
        "    mov rdi, [rsp + 32]\n"
        "    add rsp, $64\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(5, exit_code);
    return 0;
}

/* Test: Packed compare/logical path should propagate all-ones lane pattern */
static int test_integration_sse_packed_logic_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $32\n"
        "    pxor xmm0, xmm0\n"
        "    pcmpeqd xmm0, xmm0\n"
        "    pxor xmm1, xmm1\n"
        "    pcmpeqd xmm1, xmm1\n"
        "    pandn xmm1, xmm0\n"
        "    por xmm1, xmm0\n"
        "    movdqu [rsp], xmm1\n"
        "    mov rdi, [rsp]\n"
        "    add rsp, $32\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(255, exit_code);
    return 0;
}

/* Test: Packed SSE arithmetic with SIB-indexed memory operands should execute correctly */
static int test_integration_sse_packed_sib_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $128\n"
        "    mov r8, rsp\n"
        "    mov r9, $2\n"     /* vec1 at rsp + 16 */
        "    mov r10, $6\n"    /* vec2 at rsp + 48 */
        "\n"
        "    mov rax, $1\n"
        "    mov [rsp + 16], rax\n"
        "    mov rax, $2\n"
        "    mov [rsp + 24], rax\n"
        "\n"
        "    mov rax, $5\n"
        "    mov [rsp + 48], rax\n"
        "    mov rax, $7\n"
        "    mov [rsp + 56], rax\n"
        "\n"
        "    movdqu xmm0, [r8 + r9*8]\n"
        "    paddd xmm0, [r8 + r10*8]\n"
        "    movdqu [rsp + 80], xmm0\n"
        "\n"
        "    mov rdi, [rsp + 80]\n"
        "    add rsp, $128\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(6, exit_code);
    return 0;
}

/* Test: Packed compare/logical ops with SIB-indexed memory should execute correctly */
static int test_integration_sse_packed_logic_sib_runtime(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    sub rsp, $128\n"
        "    mov r8, rsp\n"
        "    mov r9, $4\n"     /* lane storage at rsp + 32 */
        "\n"
        "    pxor xmm0, xmm0\n"
        "    pcmpeqd xmm0, xmm0\n"
        "    movdqu [r8 + r9*8], xmm0\n"
        "\n"
        "    pxor xmm1, xmm1\n"
        "    por xmm1, [r8 + r9*8]\n"
        "    movdqu [rsp + 96], xmm1\n"
        "\n"
        "    mov rdi, [rsp + 96]\n"
        "    add rsp, $128\n"
        "    mov rax, $60\n"
        "    syscall\n";

    int exit_code = assemble_and_run(source);
    ASSERT_EQ(255, exit_code);
    return 0;
}

/* Test: High 8-bit register should fail when paired with REX-required register */
static int test_integration_high8_rex_conflict(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov ah, r8b\n";
    char *captured_err;
    assembler_context_t *ctx = asm_init();
    if (!ctx) return 1;

    ASSERT_EQ(0, test_capture_stderr_begin());
    int result = asm_assemble(ctx, source);
    captured_err = test_capture_stderr_end();
    asm_free(ctx);

    ASSERT_EQ(-1, result);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Cannot use AH/BH/CH/DH");
    ASSERT_STR_CONTAINS(captured_err, "Use AL/BL/CL/DL");
    free(captured_err);

    return 0;
}

/* Test: -g should emit DWARF sections into ELF output */
static int test_integration_dwarf_sections(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "after_call:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_XXXXXX";
    char bin_file[256];
    char dbg_file[300];
    int fd = mkstemp(asm_file);
    int found_info = 0;
    int found_abbrev = 0;
    int found_line = 0;
    int found_str = 0;
    int line_size_ok = 0;
    int start_name_found = 0;
    int helper_name_found = 0;
    int decl_file_attr_found = 0;
    int decl_line_attr_found = 0;
    int decl_col_attr_found = 0;
    int comp_dir_attr_found = 0;
    int type_attr_found = 0;
    int line_end_sequence_found = 0;
    int line_header_ok = 0;
    int debug_info_header_ok = 0;
    int debug_info_cu_abbrev_ok = 0;
    int debug_info_children_end_ok = 0;
    int debug_info_language_ok = 0;
    int base_type_die_count = 0;
    int pointer_type_die_count = 0;
    int subprogram_die_count = 0;
    int lexical_block_die_count = 0;
    int label_die_count = 0;
    unsigned char *bytes = NULL;

    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';
    ASSERT_TRUE(snprintf(dbg_file, sizeof(dbg_file), "%s.dbg", bin_file) < (int)sizeof(dbg_file));

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    FILE *f = fopen(bin_file, "rb");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(0, fseek(f, 0, SEEK_END));
    long size = ftell(f);
    ASSERT_TRUE(size > 0);
    ASSERT_EQ(0, fseek(f, 0, SEEK_SET));

    bytes = malloc((size_t)size);
    ASSERT_NOT_NULL(bytes);
    ASSERT_EQ(size, (long)fread(bytes, 1, (size_t)size, f));
    fclose(f);

    const test_elf64_ehdr_t *eh = (const test_elf64_ehdr_t *)bytes;
    ASSERT_TRUE(eh->e_shoff > 0);
    ASSERT_TRUE(eh->e_shnum >= 8);
    ASSERT_TRUE(eh->e_shstrndx < eh->e_shnum);

    const test_elf64_shdr_t *shdrs = (const test_elf64_shdr_t *)(bytes + eh->e_shoff);
    const test_elf64_shdr_t *shstr = &shdrs[eh->e_shstrndx];
    const char *names = (const char *)(bytes + shstr->sh_offset);

    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        const char *name = names + shdrs[i].sh_name;
        if (strcmp(name, ".debug_info") == 0) found_info = 1;
        if (strcmp(name, ".debug_info") == 0 && shdrs[i].sh_size >= 11) {
            const unsigned char *info = bytes + shdrs[i].sh_offset;
            size_t info_len = shdrs[i].sh_size;

            uint32_t unit_length = (uint32_t)info[0] |
                                   ((uint32_t)info[1] << 8) |
                                   ((uint32_t)info[2] << 16) |
                                   ((uint32_t)info[3] << 24);
            uint16_t version = (uint16_t)((uint16_t)info[4] | ((uint16_t)info[5] << 8));
            uint32_t abbrev_offset = (uint32_t)info[6] |
                                     ((uint32_t)info[7] << 8) |
                                     ((uint32_t)info[8] << 16) |
                                     ((uint32_t)info[9] << 24);
            uint8_t address_size = info[10];

            if ((size_t)unit_length + 4 <= info_len && version == 4 && abbrev_offset == 0 && address_size == 8) {
                debug_info_header_ok = 1;
            }

            if (info[11] == 1) {
                /* First DIE must be compile_unit abbrev code (1). */
                debug_info_cu_abbrev_ok = 1;

                /* Skip fixed-size compile-unit attributes. */
                size_t p = 12;
                size_t cu_attr_size = 4 + 2 + 4 + 4 + 4 + 8 + 8;
                if (p + cu_attr_size <= info_len) {
                    uint16_t language = (uint16_t)((uint16_t)info[p + 4] | ((uint16_t)info[p + 5] << 8));
                    if (language == 0x0001) {
                        debug_info_language_ok = 1;
                    }
                    p += cu_attr_size;

                    while (p < info_len) {
                        uint8_t abbrev_code = info[p++];
                        if (abbrev_code == 0) {
                            debug_info_children_end_ok = 1;
                            break;
                        }

                        if (abbrev_code == 2) {
                            /* base_type DIE: name(strp), encoding(data1), byte_size(data1) */
                            if (p + 6 > info_len) break;
                            p += 4;
                            p += 1;
                            p += 1;
                            base_type_die_count++;
                            continue;
                        }

                        if (abbrev_code == 3) {
                            /* pointer_type DIE: name(strp), byte_size(data1), type(ref4) */
                            if (p + 9 > info_len) break;
                            p += 4;
                            p += 1;
                            p += 4;
                            pointer_type_die_count++;
                            continue;
                        }

                        if (abbrev_code == 4) {
                            /* subprogram DIE: name(strp), decl_file(data1), decl_line(uleb),
                             * decl_col(uleb), external(flag), low_pc(addr), high_pc(data8), type(ref4) */
                            if (p + 5 > info_len) break;
                            p += 4; /* name */
                            p += 1; /* decl_file */

                            uint64_t uleb_val = 0;
                            p = decode_uleb128_at(info, info_len, p, &uleb_val);
                            if (p == (size_t)-1) break;
                            p = decode_uleb128_at(info, info_len, p, &uleb_val);
                            if (p == (size_t)-1) break;

                            if (p + 21 > info_len) break;
                            p += 1;  /* external */
                            p += 8;  /* low_pc */
                            p += 8;  /* high_pc */
                            p += 4;  /* type */
                            subprogram_die_count++;

                            /* subprogram children: optional lexical blocks, then 0 terminator */
                            while (p < info_len) {
                                uint8_t child_code = info[p++];
                                if (child_code == 0) {
                                    break;
                                }
                                if (child_code != 5) {
                                    p = info_len;
                                    break;
                                }
                                if (p + 16 > info_len) {
                                    p = info_len;
                                    break;
                                }
                                p += 8; /* low_pc */
                                p += 8; /* high_pc */
                                lexical_block_die_count++;
                            }
                            continue;
                        }

                        if (abbrev_code == 6) {
                            /* label DIE: name(strp), decl_file(data1), decl_line(uleb), decl_col(uleb), low_pc(addr) */
                            if (p + 5 > info_len) break;
                            p += 4;
                            p += 1;

                            uint64_t uleb_val = 0;
                            p = decode_uleb128_at(info, info_len, p, &uleb_val);
                            if (p == (size_t)-1) break;
                            p = decode_uleb128_at(info, info_len, p, &uleb_val);
                            if (p == (size_t)-1) break;

                            if (p + 8 > info_len) break;
                            p += 8;
                            label_die_count++;
                            continue;
                        }

                        break;
                    }
                }
            }
        }
        if (strcmp(name, ".debug_abbrev") == 0) {
            found_abbrev = 1;
            const unsigned char *abbr = bytes + shdrs[i].sh_offset;
            size_t abbr_len = shdrs[i].sh_size;
            for (size_t j = 0; j + 1 < abbr_len; j++) {
                if (abbr[j] == 0x3A && abbr[j + 1] == 0x0B) decl_file_attr_found = 1; /* decl_file/data1 */
                if (abbr[j] == 0x3B && abbr[j + 1] == 0x0F) decl_line_attr_found = 1; /* decl_line/udata */
                if (abbr[j] == 0x39 && abbr[j + 1] == 0x0F) decl_col_attr_found = 1; /* decl_column/udata */
                if (abbr[j] == 0x1B && abbr[j + 1] == 0x0E) comp_dir_attr_found = 1; /* comp_dir/strp */
                if (abbr[j] == 0x49 && (abbr[j + 1] == 0x13 || abbr[j + 1] == 0x10)) type_attr_found = 1; /* type/ref4|ref_addr */
            }
        }
        if (strcmp(name, ".debug_line") == 0) {
            found_line = 1;
            line_size_ok = (shdrs[i].sh_size > 16);
            if (shdrs[i].sh_size >= 16) {
                const unsigned char *line = bytes + shdrs[i].sh_offset;
                size_t line_len = shdrs[i].sh_size;

                uint32_t line_unit_length = (uint32_t)line[0] |
                                            ((uint32_t)line[1] << 8) |
                                            ((uint32_t)line[2] << 16) |
                                            ((uint32_t)line[3] << 24);
                uint16_t line_version = (uint16_t)((uint16_t)line[4] | ((uint16_t)line[5] << 8));
                uint32_t header_length = (uint32_t)line[6] |
                                         ((uint32_t)line[7] << 8) |
                                         ((uint32_t)line[8] << 16) |
                                         ((uint32_t)line[9] << 24);
                uint8_t min_inst_len = line[10];
                uint8_t max_ops_per_inst = line[11];
                uint8_t default_is_stmt = line[12];
                uint8_t line_range = line[14];
                uint8_t opcode_base = line[15];

                if ((size_t)line_unit_length + 4 <= line_len &&
                    line_version == 4 &&
                    header_length > 0 &&
                    min_inst_len == 1 &&
                    max_ops_per_inst == 1 &&
                    default_is_stmt == 1 &&
                    line_range == 14 &&
                    opcode_base == 13) {
                    line_header_ok = 1;
                }

                if (line[line_len - 3] == 0x00 && line[line_len - 2] == 0x01 && line[line_len - 1] == 0x01) {
                    line_end_sequence_found = 1;
                }
            }
        }
        if (strcmp(name, ".debug_str") == 0) {
            found_str = 1;
            const char *dbg_str = (const char *)(bytes + shdrs[i].sh_offset);
            size_t dbg_len = shdrs[i].sh_size;
            if (dbg_len >= 7) {
                for (size_t j = 0; j + 7 <= dbg_len; j++) {
                    if (memcmp(dbg_str + j, "_start", 7) == 0) {
                        start_name_found = 1;
                    }
                    if (memcmp(dbg_str + j, "helper", 7) == 0) {
                        helper_name_found = 1;
                    }
                }
            }
        }
    }

    ASSERT_TRUE(found_info);
    ASSERT_TRUE(found_abbrev);
    ASSERT_TRUE(found_line);
    ASSERT_TRUE(found_str);
    ASSERT_TRUE(line_size_ok);
    ASSERT_TRUE(start_name_found);
    ASSERT_TRUE(helper_name_found);
    ASSERT_TRUE(decl_file_attr_found);
    ASSERT_TRUE(decl_line_attr_found);
    ASSERT_TRUE(decl_col_attr_found);
    ASSERT_TRUE(comp_dir_attr_found);
    ASSERT_TRUE(line_header_ok);
    ASSERT_TRUE(line_end_sequence_found);
    ASSERT_TRUE(debug_info_header_ok);
    ASSERT_TRUE(debug_info_cu_abbrev_ok);
    ASSERT_TRUE(debug_info_children_end_ok);
    ASSERT_TRUE(debug_info_language_ok);
    ASSERT_TRUE(base_type_die_count >= 1);
    ASSERT_TRUE(pointer_type_die_count >= 1);
    ASSERT_TRUE(subprogram_die_count >= 2);
    ASSERT_TRUE(lexical_block_die_count >= 1);
    ASSERT_TRUE(label_die_count >= 1);
    ASSERT_TRUE(type_attr_found);

    free(bytes);
    unlink(asm_file);
    unlink(bin_file);
    unlink(dbg_file);
    return 0;
}

/* Test: Validate DWARF output using readelf debug dump when tool is available */
static int test_integration_dwarf_readelf_validation(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "after_call:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_readelf_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    char cmd[512];
    char line[512];
    int saw_comp_dir = 0;
    int saw_subprogram = 0;
    int saw_language = 0;
    int saw_type_attr = 0;
    int saw_lexical_block = 0;
    int saw_label_tag = 0;
    int saw_start = 0;
    int saw_line_section = 0;
    int saw_file_name = 0;

    if (system("command -v readelf >/dev/null 2>&1") != 0) {
        /* Skip in environments where readelf is unavailable. */
        return 0;
    }

    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "readelf --debug-dump=info %s 2>/dev/null", bin_file) < (int)sizeof(cmd));
    FILE *info = popen(cmd, "r");
    ASSERT_NOT_NULL(info);
    while (fgets(line, sizeof(line), info)) {
        if (strstr(line, "DW_AT_comp_dir")) saw_comp_dir = 1;
        if (strstr(line, "DW_TAG_subprogram")) saw_subprogram = 1;
        if (strstr(line, "DW_AT_language")) saw_language = 1;
        if (strstr(line, "DW_AT_type")) saw_type_attr = 1;
        if (strstr(line, "DW_TAG_lexical_block")) saw_lexical_block = 1;
        if (strstr(line, "DW_TAG_label")) saw_label_tag = 1;
        if (strstr(line, "_start")) saw_start = 1;
    }
    int info_status = pclose(info);
    ASSERT_EQ(0, info_status);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "readelf --debug-dump=line %s 2>/dev/null", bin_file) < (int)sizeof(cmd));
    FILE *line_dump = popen(cmd, "r");
    ASSERT_NOT_NULL(line_dump);
    while (fgets(line, sizeof(line), line_dump)) {
        if (strstr(line, "Line Number Statements")) saw_line_section = 1;
        if (strstr(line, "test_dbg_readelf_")) saw_file_name = 1;
    }
    int line_dump_status = pclose(line_dump);
    ASSERT_EQ(0, line_dump_status);

    ASSERT_TRUE(saw_comp_dir);
    ASSERT_TRUE(saw_subprogram);
    ASSERT_TRUE(saw_language);
    ASSERT_TRUE(saw_type_attr);
    ASSERT_TRUE(saw_lexical_block);
    ASSERT_TRUE(saw_label_tag);
    ASSERT_TRUE(saw_start);
    ASSERT_TRUE(saw_line_section);
    ASSERT_TRUE(saw_file_name);

    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: Validate DWARF output using dwarfdump/llvm-dwarfdump when available */
static int test_integration_dwarf_dwarfdump_validation(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "after_call:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_dwarfdump_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    char cmd[512];
    char line[512];
    int saw_compile_unit = 0;
    int saw_subprogram = 0;
    int saw_lexical_block = 0;
    int saw_label_tag = 0;
    int saw_start = 0;
    int saw_stmt_list = 0;
    int saw_decl_line = 0;
    const char *dump_tool = NULL;

    if (system("command -v dwarfdump >/dev/null 2>&1") == 0) {
        dump_tool = "dwarfdump";
    } else if (system("command -v llvm-dwarfdump >/dev/null 2>&1") == 0) {
        dump_tool = "llvm-dwarfdump";
    }

    if (dump_tool == NULL) {
        /* Skip in environments where no dwarfdump-compatible tool is available. */
        return 0;
    }

    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "%s %s 2>/dev/null", dump_tool, bin_file) < (int)sizeof(cmd));
    FILE *dump = popen(cmd, "r");
    ASSERT_NOT_NULL(dump);
    while (fgets(line, sizeof(line), dump)) {
        if (strstr(line, "DW_TAG_compile_unit")) saw_compile_unit = 1;
        if (strstr(line, "DW_TAG_subprogram")) saw_subprogram = 1;
        if (strstr(line, "DW_TAG_lexical_block")) saw_lexical_block = 1;
        if (strstr(line, "DW_TAG_label")) saw_label_tag = 1;
        if (strstr(line, "_start")) saw_start = 1;
        if (strstr(line, "DW_AT_stmt_list")) saw_stmt_list = 1;
        if (strstr(line, "DW_AT_decl_line")) saw_decl_line = 1;
    }
    int dump_status = pclose(dump);
    ASSERT_EQ(0, dump_status);

    ASSERT_TRUE(saw_compile_unit);
    ASSERT_TRUE(saw_subprogram);
    ASSERT_TRUE(saw_lexical_block);
    ASSERT_TRUE(saw_label_tag);
    ASSERT_TRUE(saw_start);
    ASSERT_TRUE(saw_stmt_list);
    ASSERT_TRUE(saw_decl_line);

    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: DWARF generation should fail gracefully when debug buffers are exceeded */
static int test_integration_dwarf_overflow_failure(void) {
    char asm_file[] = "/tmp/test_dbg_ovf_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    assembler_context_t *ctx;
    char *source;
    size_t cap = 220000;
    size_t len = 0;
    int func_count = 160;
    char *captured_err;

    ASSERT_TRUE(fd >= 0);

    source = malloc(cap);
    ASSERT_NOT_NULL(source);

    len += (size_t)snprintf(source + len, cap - len,
                            "section .text\n"
                            "global _start\n"
                            "_start:\n");

    for (int i = 0; i < func_count; i++) {
        len += (size_t)snprintf(source + len, cap - len,
                                "    call func_%03d_name_with_extra_padding_for_dwarf_overflow_%03d\n",
                                i, i);
    }

    len += (size_t)snprintf(source + len, cap - len,
                            "    mov rax, $60\n"
                            "    mov rdi, $0\n"
                            "    syscall\n");

    for (int i = 0; i < func_count; i++) {
        len += (size_t)snprintf(source + len, cap - len,
                                "func_%03d_name_with_extra_padding_for_dwarf_overflow_%03d:\n"
                                "    ret\n",
                                i, i);
    }

    ASSERT_TRUE(len < cap - 1);
    ASSERT_EQ((int)len, (int)write(fd, source, len));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;

    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, asm_write_elf64(ctx, bin_file));
    captured_err = test_capture_stderr_end();

    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "DWARF section data exceeded internal buffer capacity");

    free(captured_err);
    asm_free(ctx);
    free(source);
    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: .debug_line should carry known source line mappings for a simple program */
static int test_integration_dwarf_line_table_known_lines(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $1\n"
        "    mov rdi, $2\n"
        "    add rdi, $3\n"
        "    mov rax, $60\n"
        "    syscall\n";
    char asm_file[] = "/tmp/test_dbg_line_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    unsigned char *bytes = NULL;
    int saw_line_4 = 0;
    int saw_line_5 = 0;
    int saw_line_6 = 0;
    int saw_line_7 = 0;

    if (system("command -v readelf >/dev/null 2>&1") != 0) {
        return 0;
    }

    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    FILE *f = fopen(bin_file, "rb");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(0, fseek(f, 0, SEEK_END));
    long size = ftell(f);
    ASSERT_TRUE(size > 0);
    ASSERT_EQ(0, fseek(f, 0, SEEK_SET));

    bytes = malloc((size_t)size);
    ASSERT_NOT_NULL(bytes);
    ASSERT_EQ(size, (long)fread(bytes, 1, (size_t)size, f));
    fclose(f);

    const test_elf64_ehdr_t *eh = (const test_elf64_ehdr_t *)bytes;
    const test_elf64_shdr_t *shdrs = (const test_elf64_shdr_t *)(bytes + eh->e_shoff);
    const test_elf64_shdr_t *shstr = &shdrs[eh->e_shstrndx];
    const char *sec_names = (const char *)(bytes + shstr->sh_offset);
    const test_elf64_shdr_t *line_sh = NULL;

    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        const char *sname = sec_names + shdrs[i].sh_name;
        if (strcmp(sname, ".debug_line") == 0) {
            line_sh = &shdrs[i];
            break;
        }
    }

    ASSERT_NOT_NULL(line_sh);
    ASSERT_TRUE(line_sh->sh_size >= 16);

    const unsigned char *line_sec = bytes + line_sh->sh_offset;
    size_t line_len = line_sh->sh_size;
    uint32_t unit_length = (uint32_t)line_sec[0] |
                           ((uint32_t)line_sec[1] << 8) |
                           ((uint32_t)line_sec[2] << 16) |
                           ((uint32_t)line_sec[3] << 24);
    uint16_t version = (uint16_t)((uint16_t)line_sec[4] | ((uint16_t)line_sec[5] << 8));
    uint32_t header_length = (uint32_t)line_sec[6] |
                             ((uint32_t)line_sec[7] << 8) |
                             ((uint32_t)line_sec[8] << 16) |
                             ((uint32_t)line_sec[9] << 24);
    ASSERT_EQ(4, version);
    ASSERT_TRUE((size_t)unit_length + 4 <= line_len);

    size_t p = 10 + header_length;
    int current_line = 1;

    while (p < line_len) {
        uint8_t op = line_sec[p++];
        if (op == 0) {
            if (p >= line_len) break;
            uint8_t ext_len = line_sec[p++];
            if (p + ext_len > line_len) break;
            if (ext_len > 0 && line_sec[p] == 1) {
                break; /* DW_LNE_end_sequence */
            }
            p += ext_len;
            continue;
        }

        if (op == 2) {
            uint64_t adv = 0;
            p = decode_uleb128_at(line_sec, line_len, p, &adv);
            if (p == (size_t)-1) break;
            continue;
        }

        if (op == 3) {
            int64_t delta = 0;
            p = decode_sleb128_at(line_sec, line_len, p, &delta);
            if (p == (size_t)-1) break;
            current_line += (int)delta;
            continue;
        }

        if (op == 1) {
            if (current_line == 4) saw_line_4 = 1;
            if (current_line == 5) saw_line_5 = 1;
            if (current_line == 6) saw_line_6 = 1;
            if (current_line == 7) saw_line_7 = 1;
            continue;
        }

        break;
    }

    ASSERT_TRUE(saw_line_4);
    ASSERT_TRUE(saw_line_5);
    ASSERT_TRUE(saw_line_6);
    ASSERT_TRUE(saw_line_7);

    free(bytes);
    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: DWARF DIE names should stay consistent with text symbols in .symtab */
static int test_integration_dwarf_symbol_table_crosscheck(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "after_call:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_sym_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    char cmd[512];
    char line[512];
    unsigned char *bytes = NULL;
    int have_start = 0;
    int have_helper = 0;
    int have_after_call = 0;
    int dwarf_has_start = 0;
    int dwarf_has_helper = 0;
    int dwarf_has_after_call = 0;

    if (system("command -v readelf >/dev/null 2>&1") != 0) {
        return 0;
    }

    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    FILE *f = fopen(bin_file, "rb");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(0, fseek(f, 0, SEEK_END));
    long size = ftell(f);
    ASSERT_TRUE(size > 0);
    ASSERT_EQ(0, fseek(f, 0, SEEK_SET));

    bytes = malloc((size_t)size);
    ASSERT_NOT_NULL(bytes);
    ASSERT_EQ(size, (long)fread(bytes, 1, (size_t)size, f));
    fclose(f);

    const test_elf64_ehdr_t *eh = (const test_elf64_ehdr_t *)bytes;
    const test_elf64_shdr_t *shdrs = (const test_elf64_shdr_t *)(bytes + eh->e_shoff);
    const test_elf64_shdr_t *shstr = &shdrs[eh->e_shstrndx];
    const char *sec_names = (const char *)(bytes + shstr->sh_offset);
    const test_elf64_shdr_t *symtab_sh = NULL;
    const test_elf64_shdr_t *strtab_sh = NULL;

    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        const char *sname = sec_names + shdrs[i].sh_name;
        if (strcmp(sname, ".symtab") == 0) symtab_sh = &shdrs[i];
        if (strcmp(sname, ".strtab") == 0) strtab_sh = &shdrs[i];
    }

    if (!symtab_sh || !strtab_sh) {
        free(bytes);
        unlink(asm_file);
        unlink(bin_file);
        return 0;
    }

    ASSERT_TRUE(symtab_sh->sh_entsize == sizeof(test_elf64_sym_t));

    const test_elf64_sym_t *syms = (const test_elf64_sym_t *)(bytes + symtab_sh->sh_offset);
    size_t sym_count = (size_t)(symtab_sh->sh_size / symtab_sh->sh_entsize);
    const char *strtab = (const char *)(bytes + strtab_sh->sh_offset);
    size_t strtab_size = strtab_sh->sh_size;

    for (size_t i = 0; i < sym_count; i++) {
        uint32_t off = syms[i].st_name;
        if (off >= strtab_size) continue;
        const char *name = strtab + off;
        if (strcmp(name, "_start") == 0) have_start = 1;
        if (strcmp(name, "helper") == 0) have_helper = 1;
        if (strcmp(name, "after_call") == 0) have_after_call = 1;
    }

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "readelf --debug-dump=info %s 2>/dev/null", bin_file) < (int)sizeof(cmd));
    FILE *info = popen(cmd, "r");
    ASSERT_NOT_NULL(info);
    while (fgets(line, sizeof(line), info)) {
        if (strstr(line, "_start")) dwarf_has_start = 1;
        if (strstr(line, "helper")) dwarf_has_helper = 1;
        if (strstr(line, "after_call")) dwarf_has_after_call = 1;
    }
    int info_status = pclose(info);
    ASSERT_EQ(0, info_status);

    ASSERT_TRUE(have_start);
    ASSERT_TRUE(have_helper);
    ASSERT_TRUE(have_after_call);
    ASSERT_TRUE(dwarf_has_start);
    ASSERT_TRUE(dwarf_has_helper);
    ASSERT_TRUE(dwarf_has_after_call);

    free(bytes);
    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: Large (50+ function) DWARF generation should succeed and remain structured */
static int test_integration_dwarf_large_program_success(void) {
    char asm_file[] = "/tmp/test_dbg_large_ok_XXXXXX";
    char bin_file[256];
    int fd = mkstemp(asm_file);
    assembler_context_t *ctx;
    char *source;
    size_t cap = 160000;
    size_t len = 0;
    int func_count = 64;
    unsigned char *bytes = NULL;

    ASSERT_TRUE(fd >= 0);

    source = malloc(cap);
    ASSERT_NOT_NULL(source);

    len += (size_t)snprintf(source + len, cap - len,
                            "section .text\n"
                            "global _start\n"
                            "_start:\n");

    for (int i = 0; i < func_count; i++) {
        len += (size_t)snprintf(source + len, cap - len,
                                "    call f_%03d\n", i);
    }

    len += (size_t)snprintf(source + len, cap - len,
                            "    mov rax, $60\n"
                            "    mov rdi, $0\n"
                            "    syscall\n");

    for (int i = 0; i < func_count; i++) {
        len += (size_t)snprintf(source + len, cap - len,
                                "f_%03d:\n"
                                "    ret\n", i);
    }

    ASSERT_TRUE(len < cap - 1);
    ASSERT_EQ((int)len, (int)write(fd, source, len));
    close(fd);

    strncpy(bin_file, asm_file, sizeof(bin_file) - 1);
    bin_file[sizeof(bin_file) - 1] = '\0';
    char *dot = strrchr(bin_file, '.');
    if (dot) *dot = '\0';

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ctx->emit_debug_map = true;

    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_EQ(0, asm_write_elf64(ctx, bin_file));
    asm_free(ctx);

    FILE *f = fopen(bin_file, "rb");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(0, fseek(f, 0, SEEK_END));
    long size = ftell(f);
    ASSERT_TRUE(size > 0);
    ASSERT_EQ(0, fseek(f, 0, SEEK_SET));

    bytes = malloc((size_t)size);
    ASSERT_NOT_NULL(bytes);
    ASSERT_EQ(size, (long)fread(bytes, 1, (size_t)size, f));
    fclose(f);

    const test_elf64_ehdr_t *eh = (const test_elf64_ehdr_t *)bytes;
    const test_elf64_shdr_t *shdrs = (const test_elf64_shdr_t *)(bytes + eh->e_shoff);
    const test_elf64_shdr_t *shstr = &shdrs[eh->e_shstrndx];
    const char *names = (const char *)(bytes + shstr->sh_offset);
    size_t debug_info_size = 0;
    size_t debug_line_size = 0;

    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        const char *name = names + shdrs[i].sh_name;
        if (strcmp(name, ".debug_info") == 0) debug_info_size = shdrs[i].sh_size;
        if (strcmp(name, ".debug_line") == 0) debug_line_size = shdrs[i].sh_size;
    }

    ASSERT_TRUE(debug_info_size > 0);
    ASSERT_TRUE(debug_line_size > 0);
    ASSERT_TRUE(debug_info_size < 16384);
    ASSERT_TRUE(debug_line_size < 65536);

    free(bytes);
    free(source);
    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: All supported output formats should write valid artifacts */
static int test_integration_output_formats_compile(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n";
    char asm_file[] = "/tmp/test_fmt_src_XXXXXX";
    char out_elf[] = "/tmp/test_fmt_elf_XXXXXX";
    char out_bin[] = "/tmp/test_fmt_bin_XXXXXX";
    char out_hex[] = "/tmp/test_fmt_hex_XXXXXX";
    int asm_fd = -1;
    int out_fd = -1;
    assembler_context_t *ctx = NULL;
    struct stat st;

    asm_fd = mkstemp(asm_file);
    ASSERT_TRUE(asm_fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(asm_fd, source, strlen(source)));
    close(asm_fd);

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));
    ASSERT_TRUE(ctx->text_size > 0);

    out_fd = mkstemp(out_elf);
    ASSERT_TRUE(out_fd >= 0);
    close(out_fd);
    ASSERT_EQ(0, asm_write_elf64(ctx, out_elf));
    ASSERT_EQ(0, stat(out_elf, &st));
    ASSERT_TRUE(st.st_size > 0);
    {
        unsigned char magic[4];
        FILE *f = fopen(out_elf, "rb");
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(4, (int)fread(magic, 1, sizeof(magic), f));
        fclose(f);
        ASSERT_EQ(0x7f, magic[0]);
        ASSERT_EQ('E', magic[1]);
        ASSERT_EQ('L', magic[2]);
        ASSERT_EQ('F', magic[3]);
    }

    out_fd = mkstemp(out_bin);
    ASSERT_TRUE(out_fd >= 0);
    close(out_fd);
    ASSERT_EQ(0, asm_write_binary(ctx, out_bin));
    ASSERT_EQ(0, stat(out_bin, &st));
    ASSERT_EQ((long long)ctx->text_size, (long long)st.st_size);

    out_fd = mkstemp(out_hex);
    ASSERT_TRUE(out_fd >= 0);
    close(out_fd);
    ASSERT_EQ(0, asm_write_hex(ctx, out_hex));
    ASSERT_EQ(0, stat(out_hex, &st));
    ASSERT_TRUE(st.st_size > 0);
    {
        FILE *f = fopen(out_hex, "r");
        int c;
        int seen_hex_digit = 0;
        ASSERT_NOT_NULL(f);
        while ((c = fgetc(f)) != EOF) {
            if (isxdigit((unsigned char)c)) {
                seen_hex_digit = 1;
                continue;
            }
            ASSERT_TRUE(c == ' ' || c == '\n' || c == '\r' || c == '\t');
        }
        fclose(f);
        ASSERT_TRUE(seen_hex_digit);
    }

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_elf);
    unlink(out_bin);
    unlink(out_hex);
    return 0;
}

/* Test: listing generation should produce a readable file with expected sections */
static int test_integration_listing_file_sections(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n";
    char asm_file[256];
    char out_file[256];
    char lst_file[256];
    assembler_context_t *ctx = NULL;
    struct stat st;
    char line[512];
    int saw_header = 0;
    int saw_symbols = 0;

    ASSERT_EQ(0, assemble_and_write_listing(source,
                                            asm_file, sizeof(asm_file),
                                            out_file, sizeof(out_file),
                                            lst_file, sizeof(lst_file),
                                            &ctx));
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(0, stat(lst_file, &st));
    ASSERT_TRUE(st.st_size > 0);

    FILE *f = fopen(lst_file, "r");
    ASSERT_NOT_NULL(f);
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Line   Address  Machine Code") != NULL) {
            saw_header = 1;
        }
        if (strstr(line, "; Symbol Table") != NULL) {
            saw_symbols = 1;
        }
    }
    fclose(f);

    ASSERT_TRUE(saw_header);
    ASSERT_TRUE(saw_symbols);

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_file);
    unlink(lst_file);
    return 0;
}

/* Test: listing should contain instruction/data rows with source snippets */
static int test_integration_listing_rows_present(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "section .data\n"
        "msg: db \"OK\", 10\n";
    char asm_file[256];
    char out_file[256];
    char lst_file[256];
    assembler_context_t *ctx = NULL;
    char line[512];
    int row_count = 0;
    int saw_mov = 0;

    ASSERT_EQ(0, assemble_and_write_listing(source,
                                            asm_file, sizeof(asm_file),
                                            out_file, sizeof(out_file),
                                            lst_file, sizeof(lst_file),
                                            &ctx));
    ASSERT_NOT_NULL(ctx);

    FILE *f = fopen(lst_file, "r");
    ASSERT_NOT_NULL(f);
    while (fgets(line, sizeof(line), f)) {
        listing_row_t row;
        if (parse_listing_row(line, &row)) {
            row_count++;
            if (strstr(line, "mov rax") != NULL) {
                saw_mov = 1;
            }
        }
    }
    fclose(f);

    ASSERT_TRUE(row_count >= 3);
    ASSERT_TRUE(saw_mov);

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_file);
    unlink(lst_file);
    return 0;
}

/* Test: listing row addresses should be monotonic non-decreasing */
static int test_integration_listing_address_monotonic(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $1\n"
        "    mov rdi, $1\n"
        "    syscall\n"
        "section .data\n"
        "v: dq 0x11223344\n";
    char asm_file[256];
    char out_file[256];
    char lst_file[256];
    assembler_context_t *ctx = NULL;
    char line[512];
    uint64_t prev = 0;
    int saw_any = 0;

    ASSERT_EQ(0, assemble_and_write_listing(source,
                                            asm_file, sizeof(asm_file),
                                            out_file, sizeof(out_file),
                                            lst_file, sizeof(lst_file),
                                            &ctx));
    ASSERT_NOT_NULL(ctx);

    FILE *f = fopen(lst_file, "r");
    ASSERT_NOT_NULL(f);
    while (fgets(line, sizeof(line), f)) {
        listing_row_t row;
        if (!parse_listing_row(line, &row)) {
            continue;
        }
        if (!saw_any) {
            prev = row.address;
            saw_any = 1;
            continue;
        }
        ASSERT_TRUE(row.address >= prev);
        prev = row.address;
    }
    fclose(f);

    ASSERT_TRUE(saw_any);

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_file);
    unlink(lst_file);
    return 0;
}

/* Test: listing bytes should match assembled text/data buffers by absolute address */
static int test_integration_listing_bytes_match_buffers(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "section .data\n"
        "msg: db \"ABC\", 10\n";
    char asm_file[256];
    char out_file[256];
    char lst_file[256];
    assembler_context_t *ctx = NULL;
    char line[512];
    int matched_rows = 0;

    ASSERT_EQ(0, assemble_and_write_listing(source,
                                            asm_file, sizeof(asm_file),
                                            out_file, sizeof(out_file),
                                            lst_file, sizeof(lst_file),
                                            &ctx));
    ASSERT_NOT_NULL(ctx);

    FILE *f = fopen(lst_file, "r");
    ASSERT_NOT_NULL(f);
    while (fgets(line, sizeof(line), f)) {
        listing_row_t row;
        if (!parse_listing_row(line, &row)) {
            continue;
        }

        uint64_t offset = row.address - ctx->base_address;
        for (int i = 0; i < row.byte_count; i++) {
            uint8_t expected;
            if (offset + (uint64_t)i < (uint64_t)ctx->text_size) {
                expected = ctx->text_section[offset + (uint64_t)i];
            } else {
                uint64_t data_off = (offset + (uint64_t)i) - (uint64_t)ctx->text_size;
                ASSERT_TRUE(data_off < (uint64_t)ctx->data_size);
                expected = ctx->data_section[data_off];
            }
            ASSERT_EQ_HEX(expected, row.bytes[i]);
        }
        matched_rows++;
    }
    fclose(f);

    ASSERT_TRUE(matched_rows > 0);

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_file);
    unlink(lst_file);
    return 0;
}

/* Test: listing symbol appendix should contain key symbols with section labels */
static int test_integration_listing_symbol_table_entries(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "section .data\n"
        "msg: db \"Hello\", 0\n";
    char asm_file[256];
    char out_file[256];
    char lst_file[256];
    assembler_context_t *ctx = NULL;
    char line[512];
    int saw_start = 0;
    int saw_msg = 0;

    ASSERT_EQ(0, assemble_and_write_listing(source,
                                            asm_file, sizeof(asm_file),
                                            out_file, sizeof(out_file),
                                            lst_file, sizeof(lst_file),
                                            &ctx));
    ASSERT_NOT_NULL(ctx);

    FILE *f = fopen(lst_file, "r");
    ASSERT_NOT_NULL(f);
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "; _start") != NULL && strstr(line, "text") != NULL) {
            saw_start = 1;
        }
        if (strstr(line, "; msg") != NULL) {
            saw_msg = 1;
        }
    }
    fclose(f);

    ASSERT_TRUE(saw_start);
    ASSERT_TRUE(saw_msg);

    asm_free(ctx);
    unlink(asm_file);
    unlink(out_file);
    unlink(lst_file);
    return 0;
}

/* Test: compare disassembler mnemonic coverage against objdump for a representative program */
static int test_integration_disassembler_objdump_semantic_compare(void) {
    const char *mnemonics[] = {"push", "mov", "sub", "cmp", "jle", "xor", "movsxd"};
    const char *target_bin = "./bin/x86_64-asm";
    char cmd[512];
    char line[512];
    int saw_ours[7] = {0, 0, 0, 0, 0, 0, 0};
    int saw_objdump[7] = {0, 0, 0, 0, 0, 0, 0};

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ASSERT_TRUE(access(target_bin, X_OK) == 0);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "./bin/x86_64-asm --disassemble %s 2>/dev/null", target_bin) < (int)sizeof(cmd));
    {
        FILE *ours = popen(cmd, "r");
        ASSERT_NOT_NULL(ours);
        while (fgets(line, sizeof(line), ours)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_ours[i] = 1;
                }
            }
        }
        {
            int ours_status = pclose(ours);
            ASSERT_EQ(0, ours_status);
        }
    }

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd), "objdump -d -M intel %s 2>/dev/null", target_bin) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    return 0;
}

/* Test: compare 0x66 operand-override semantic mnemonics against objdump on controlled bytes */
static int test_integration_disassembler_objdump_operand_override_fixture(void) {
    static const uint8_t code[] = {
        0x66, 0x0f, 0xbe, 0xc1,
        0x66, 0x0f, 0xbf, 0xc2,
        0x66, 0x0f, 0xaf, 0xc1,
        0x66, 0x0f, 0xa3, 0xc1,
        0x66, 0x0f, 0xab, 0xc1,
        0x66, 0x0f, 0xb3, 0xc1,
        0x66, 0x0f, 0xbb, 0xc1
    };
    const char *mnemonics[] = {"movsx", "imul", "bt", "bts", "btr", "btc"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_raw_XXXXXX";
    int fd;
    int saw_ours[6] = {0, 0, 0, 0, 0, 0};
    int saw_objdump[6] = {0, 0, 0, 0, 0, 0};
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x500000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare 0F BA immediate bit-test semantic mnemonics against objdump on controlled bytes */
static int test_integration_disassembler_objdump_bit_test_immediate_fixture(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xba, 0xe0, 0x05,
        0x48, 0x0f, 0xba, 0xe9, 0x07,
        0x48, 0x0f, 0xba, 0xf2, 0x03,
        0x48, 0x0f, 0xba, 0xfb, 0x01,
        0x48, 0x0f, 0xba, 0xac, 0x8b, 0x20, 0x00, 0x00, 0x00, 0x09,
        0x66, 0x0f, 0xba, 0xe1, 0x04
    };
    const char *mnemonics[] = {"bt", "bts", "btr", "btc"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_ba_raw_XXXXXX";
    int fd;
    int saw_ours[4] = {0, 0, 0, 0};
    int saw_objdump[4] = {0, 0, 0, 0};
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x510000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare bsf/bsr/test semantic mnemonics against objdump on controlled bytes */
static int test_integration_disassembler_objdump_bit_scan_and_test_fixture(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xbc, 0xc3,
        0x48, 0x0f, 0xbd, 0x45, 0xf8,
        0x48, 0x85, 0xc8,
        0x48, 0x85, 0x8c, 0x8b, 0x10, 0x00, 0x00, 0x00,
        0x66, 0x0f, 0xbc, 0xc1,
        0x66, 0x0f, 0xbd, 0xc2,
        0x66, 0x85, 0xc8
    };
    const char *mnemonics[] = {"bsf", "bsr", "test"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_bscan_raw_XXXXXX";
    int fd;
    int saw_ours[3] = {0, 0, 0};
    int saw_objdump[3] = {0, 0, 0};
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x520000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare xadd/cmpxchg semantic mnemonics against objdump with RIP-relative and SIB addressing */
static int test_integration_disassembler_objdump_xadd_cmpxchg_fixture(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xc1, 0xd8,
        0x0f, 0xc0, 0xc1,
        0x48, 0x0f, 0xb1, 0xd1,
        0x66, 0x0f, 0xb1, 0xc8,
        0x48, 0x0f, 0xc1, 0x0d, 0x20, 0x00, 0x00, 0x00,
        0x48, 0x0f, 0xb1, 0x15, 0x10, 0x00, 0x00, 0x00,
        0x0f, 0xc1, 0x44, 0x8b, 0x10
    };
    const char *mnemonics[] = {"xadd", "cmpxchg"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_xadd_raw_XXXXXX";
    int fd;
    int saw_ours[2] = {0, 0};
    int saw_objdump[2] = {0, 0};
    int saw_rip_ours = 0;
    int saw_rip_objdump = 0;
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x530000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }
    if (strstr(ours_text, "[rip+") != NULL || strstr(ours_text, "[rip-") != NULL) {
        saw_rip_ours = 1;
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
            if (strstr(line, "[rip+") != NULL || strstr(line, "[rip-") != NULL) {
                saw_rip_objdump = 1;
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }
    ASSERT_TRUE(saw_rip_ours);
    ASSERT_TRUE(saw_rip_objdump);

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare setcc and movzx/movsx edge semantic mnemonics against objdump */
static int test_integration_disassembler_objdump_setcc_movx_edge_fixture(void) {
    static const uint8_t code[] = {
        0x0f, 0x94, 0xc4,
        0x40, 0x0f, 0x94, 0xc4,
        0x44, 0x0f, 0xb6, 0xc4,
        0x0f, 0xbe, 0xc4,
        0x40, 0x0f, 0xbe, 0xc4,
        0x66, 0x0f, 0xb7, 0xc1,
        0x0f, 0x95, 0x05, 0x34, 0x12, 0x00, 0x00,
        0x48, 0x0f, 0xb6, 0x0d, 0x10, 0x00, 0x00, 0x00
    };
    const char *mnemonics[] = {"set", "movzx", "movsx"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_setcc_movx_raw_XXXXXX";
    int fd;
    int saw_ours[3] = {0, 0, 0};
    int saw_objdump[3] = {0, 0, 0};
    int saw_rip_ours = 0;
    int saw_rip_objdump = 0;
    int saw_spl_ours = 0;
    int saw_spl_objdump = 0;
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x540000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }
    if (strstr(ours_text, "[rip+") != NULL || strstr(ours_text, "[rip-") != NULL) {
        saw_rip_ours = 1;
    }
    if (strstr(ours_text, "spl") != NULL) {
        saw_spl_ours = 1;
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            if (strstr(line, "set") != NULL) {
                saw_objdump[0] = 1;
            }
            if (strstr(line, "movzx") != NULL) {
                saw_objdump[1] = 1;
            }
            if (strstr(line, "movsx") != NULL) {
                saw_objdump[2] = 1;
            }
            if (strstr(line, "[rip+") != NULL || strstr(line, "[rip-") != NULL) {
                saw_rip_objdump = 1;
            }
            if (strstr(line, "spl") != NULL) {
                saw_spl_objdump = 1;
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }
    ASSERT_TRUE(saw_rip_ours);
    ASSERT_TRUE(saw_rip_objdump);
    ASSERT_TRUE(saw_spl_ours);
    ASSERT_TRUE(saw_spl_objdump);

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare multi-byte NOP and group-0x80 semantic mnemonics against objdump */
static int test_integration_disassembler_objdump_nop_group80_fixture(void) {
    static const uint8_t code[] = {
        0x0f, 0x1f, 0x00,
        0x66, 0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0xc0, 0x01,
        0x80, 0xe1, 0x7f,
        0x80, 0x6d, 0xf8, 0x80,
        0x80, 0x3d, 0x10, 0x00, 0x00, 0x00, 0xff
    };
    const char *mnemonics[] = {"nop", "add", "and", "sub", "cmp"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_nop80_raw_XXXXXX";
    int fd;
    int saw_ours[5] = {0, 0, 0, 0, 0};
    int saw_objdump[5] = {0, 0, 0, 0, 0};
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x550000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: compare group2/group3 and byte-MOV semantic mnemonics against objdump */
static int test_integration_disassembler_objdump_group2_group3_fixture(void) {
    static const uint8_t code[] = {
        0x88, 0xc8,
        0x8a, 0xd9,
        0x44, 0x88, 0xc4,
        0xc0, 0xe8, 0x04,
        0x48, 0xc1, 0xe9, 0x08,
        0x66, 0xc1, 0xf9, 0x01,
        0xf6, 0xc1, 0x0f,
        0xf6, 0xd0,
        0xf6, 0xd8,
        0xf7, 0xc1, 0x34, 0x12, 0x00, 0x00,
        0x66, 0xf7, 0xc1, 0xcd, 0xab,
        0xf7, 0xd1,
        0xf7, 0xd9
    };
    const char *mnemonics[] = {"mov", "shr", "sar", "test", "not", "neg"};
    char cmd[512];
    char line[512];
    char raw_file[] = "/tmp/test_disasm_grp23_raw_XXXXXX";
    int fd;
    int saw_ours[6] = {0, 0, 0, 0, 0, 0};
    int saw_objdump[6] = {0, 0, 0, 0, 0, 0};
    char *ours_text;

    if (system("command -v objdump >/dev/null 2>&1") != 0) {
        return 0;
    }

    ours_text = disassemble_buffer_to_string(code, sizeof(code), 0x560000U);
    ASSERT_NOT_NULL(ours_text);
    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
    }

    fd = mkstemp(raw_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)sizeof(code), (int)write(fd, code, sizeof(code)));
    close(fd);

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    {
        FILE *objdump = popen(cmd, "r");
        ASSERT_NOT_NULL(objdump);
        while (fgets(line, sizeof(line), objdump)) {
            for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
                if (strstr(line, mnemonics[i]) != NULL) {
                    saw_objdump[i] = 1;
                }
            }
        }
        {
            int objdump_status = pclose(objdump);
            ASSERT_EQ(0, objdump_status);
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_objdump[i]);
        ASSERT_TRUE(saw_ours[i]);
    }

    free(ours_text);
    unlink(raw_file);
    return 0;
}

/* Test: assemble source -> disassemble output -> semantic compare with source intent and objdump */
static int test_integration_disassembler_source_roundtrip_semantic(void) {
    typedef struct {
        uint32_t p_type;
        uint32_t p_flags;
        uint64_t p_offset;
        uint64_t p_vaddr;
        uint64_t p_paddr;
        uint64_t p_filesz;
        uint64_t p_memsz;
        uint64_t p_align;
    } test_elf64_phdr_t;

    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    mov rax, $60\n"
        "    xor rdi, rdi\n"
        "    syscall\n";
    const char *mnemonics[] = {"mov", "xor", "syscall"};
    char asm_file[] = "/tmp/test_roundtrip_src_XXXXXX";
    char out_file[512];
    char raw_file[] = "/tmp/test_roundtrip_raw_XXXXXX";
    char cmd[1024];
    assembler_context_t *ctx = NULL;
    char *ours_text = NULL;
    char *objdump_text = NULL;
    int saw_ours[3] = {0, 0, 0};
    int saw_objdump[3] = {0, 0, 0};
    uint8_t *bytes = NULL;
    const uint8_t *seg_bytes = NULL;
    size_t seg_size = 0;
    uint64_t seg_addr = 0;
    int fd;

    fd = mkstemp(asm_file);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ((int)strlen(source), (int)write(fd, source, strlen(source)));
    close(fd);

    ASSERT_TRUE(snprintf(out_file, sizeof(out_file), "%s.out", asm_file) < (int)sizeof(out_file));

    ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    ASSERT_EQ(0, asm_assemble_file(ctx, asm_file));

    ASSERT_EQ(0, asm_write_elf64(ctx, out_file));
    asm_free(ctx);
    ctx = NULL;

    {
        FILE *f = fopen(out_file, "rb");
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(0, fseek(f, 0, SEEK_END));
        long file_size = ftell(f);
        ASSERT_TRUE(file_size > 0);
        ASSERT_EQ(0, fseek(f, 0, SEEK_SET));

        bytes = (uint8_t *)malloc((size_t)file_size);
        ASSERT_NOT_NULL(bytes);
        ASSERT_EQ(file_size, (long)fread(bytes, 1, (size_t)file_size, f));
        fclose(f);

        const test_elf64_ehdr_t *eh = (const test_elf64_ehdr_t *)bytes;
        ASSERT_TRUE(eh->e_phoff > 0);
        ASSERT_TRUE(eh->e_phnum > 0);

        const test_elf64_phdr_t *phdrs = (const test_elf64_phdr_t *)(bytes + eh->e_phoff);
        for (uint16_t i = 0; i < eh->e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD && (phdrs[i].p_flags & PF_X) != 0 && phdrs[i].p_filesz > 0) {
                seg_bytes = bytes + phdrs[i].p_offset;
                seg_size = (size_t)phdrs[i].p_filesz;
                seg_addr = phdrs[i].p_vaddr;
                break;
            }
        }
    }
    ASSERT_NOT_NULL(seg_bytes);
    ASSERT_TRUE(seg_size > 0U);

    ours_text = disassemble_buffer_to_string(seg_bytes, seg_size, seg_addr);
    ASSERT_NOT_NULL(ours_text);

    {
        int raw_fd = mkstemp(raw_file);
        ASSERT_TRUE(raw_fd >= 0);
        ASSERT_EQ((int)seg_size, (int)write(raw_fd, seg_bytes, seg_size));
        close(raw_fd);
    }

    ASSERT_TRUE(snprintf(cmd, sizeof(cmd),
                         "objdump -D -b binary -m i386:x86-64 -M intel %s 2>/dev/null",
                         raw_file) < (int)sizeof(cmd));
    objdump_text = capture_command_output(cmd);
    ASSERT_NOT_NULL(objdump_text);

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        if (strstr(ours_text, mnemonics[i]) != NULL) {
            saw_ours[i] = 1;
        }
        if (strstr(objdump_text, mnemonics[i]) != NULL) {
            saw_objdump[i] = 1;
        }
    }

    for (size_t i = 0; i < (sizeof(mnemonics) / sizeof(mnemonics[0])); i++) {
        ASSERT_TRUE(saw_ours[i]);
        ASSERT_TRUE(saw_objdump[i]);
    }

    free(ours_text);
    free(objdump_text);
    free(bytes);
    if (ctx) {
        asm_free(ctx);
    }
    unlink(asm_file);
    unlink(out_file);
    unlink(raw_file);
    return 0;
}

/* Test: stress fixtures should assemble successfully and produce non-empty artifacts */
static int test_integration_stress_fixtures_compile_nonempty(void) {
    const char *fixtures[] = {
        "examples/stress_100_functions.asm",
        "examples/stress_1000_labels.asm",
        "examples/stress_50kb_mixed.asm",
        "examples/stress_macro_nested.asm",
        "examples/stress_sse_broad.asm",
        "examples/stress_addressing_matrix.asm"
    };

    for (size_t i = 0; i < (sizeof(fixtures) / sizeof(fixtures[0])); i++) {
        ASSERT_EQ(0, access(fixtures[i], R_OK));
        ASSERT_EQ(0, assemble_fixture_and_verify_nonempty_output(fixtures[i]));
    }

    return 0;
}

/* Test Suite: Integration Tests */
TEST_SUITE(integration) {
    TEST(integration_exit);
    TEST(integration_arithmetic);
    TEST(integration_registers);
    TEST(integration_conditional);
    TEST(integration_loop);
    TEST(integration_local_labels_scoped);
    TEST(integration_anonymous_labels);
    TEST(integration_pushpop);
    TEST(integration_callret);
    TEST(integration_enter_legacy_signext);
    TEST(integration_8bit);
    TEST(integration_data);
    TEST(integration_label_arithmetic_equ);
    TEST(integration_weak_hidden_attributes);
    TEST(integration_named_sections);
    TEST(integration_segment_alias);
    TEST(integration_preprocessor_conditionals);
    TEST(integration_empty);
    TEST(integration_invalid);
    TEST(integration_enter_invalid_operands);
    TEST(integration_enter_invalid_nesting);
    TEST(integration_failure_scenarios_matrix);
    TEST(integration_sectionless_entrypoint);
    TEST(integration_include_relative);
    TEST(integration_include_circular_fails);
    TEST(integration_include_duplicate_symbol_handled);
    TEST(integration_include_depth_overflow_fails);
    TEST(integration_sse_smoke);
    TEST(integration_sse_mixed_controlflow);
    TEST(integration_sse_scalar_to_gpr_runtime);
    TEST(integration_sse_all_xmm_registers_runtime);
    TEST(integration_sse_sequence_runtime);
    TEST(integration_sse_function_call_preserve_runtime);
    TEST(integration_sse_packed_arith_runtime);
    TEST(integration_sse_packed_logic_runtime);
    TEST(integration_sse_packed_sib_runtime);
    TEST(integration_sse_packed_logic_sib_runtime);
    TEST(integration_high8_rex_conflict);
    TEST(integration_dwarf_sections);
    TEST(integration_dwarf_readelf_validation);
    TEST(integration_dwarf_dwarfdump_validation);
    TEST(integration_dwarf_overflow_failure);
    TEST(integration_dwarf_line_table_known_lines);
    TEST(integration_dwarf_symbol_table_crosscheck);
    TEST(integration_dwarf_large_program_success);
    TEST(integration_output_formats_compile);
    TEST(integration_listing_file_sections);
    TEST(integration_listing_rows_present);
    TEST(integration_listing_address_monotonic);
    TEST(integration_listing_bytes_match_buffers);
    TEST(integration_listing_symbol_table_entries);
    TEST(integration_disassembler_objdump_semantic_compare);
    TEST(integration_disassembler_objdump_operand_override_fixture);
    TEST(integration_disassembler_objdump_bit_test_immediate_fixture);
    TEST(integration_disassembler_objdump_bit_scan_and_test_fixture);
    TEST(integration_disassembler_objdump_xadd_cmpxchg_fixture);
    TEST(integration_disassembler_objdump_setcc_movx_edge_fixture);
    TEST(integration_disassembler_objdump_nop_group80_fixture);
    TEST(integration_disassembler_objdump_group2_group3_fixture);
    TEST(integration_disassembler_source_roundtrip_semantic);
    TEST(integration_stress_fixtures_compile_nonempty);
}

/* Main entry point */
int main(void) {
    test_init();
    RUN_TEST_SUITE(integration);
    test_report();
    return test_final_status();
}
