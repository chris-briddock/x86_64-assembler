/**
 * Integration Tests
 * End-to-end testing of the assembler pipeline
 */

#include "test_framework.h"
#include "../src/x86_64_asm.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

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
    char asm_file[] = "/tmp/test_XXXXXX.asm";
    
    int fd = mkstemps(asm_file, 4);
    if (fd < 0) return -1;
    
    /* Write assembly to temp file */
    write(fd, source, strlen(source));
    close(fd);
    
    int result = assemble_file_and_run(asm_file);
    unlink(asm_file);
    return result;
}

/* Test: Simple exit program */
int test_integration_exit(void) {
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
int test_integration_arithmetic(void) {
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
int test_integration_registers(void) {
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
int test_integration_conditional(void) {
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
int test_integration_loop(void) {
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

/* Test: Push and pop */
int test_integration_pushpop(void) {
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
int test_integration_callret(void) {
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

/* Test: 8-bit register operations */
int test_integration_8bit(void) {
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
int test_integration_data(void) {
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

/* Test: Empty program should fail gracefully */
int test_integration_empty(void) {
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
int test_integration_invalid(void) {
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

/* Regression test: sectionless source should still enter at _start */
int test_integration_sectionless_entrypoint(void) {
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
int test_integration_include_relative(void) {
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

/* Test: SSE instructions should assemble and execute in a simple program */
int test_integration_sse_smoke(void) {
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
int test_integration_sse_mixed_controlflow(void) {
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

/* Test: High 8-bit register should fail when paired with REX-required register */
int test_integration_high8_rex_conflict(void) {
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
int test_integration_dwarf_sections(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_XXXXXX.asm";
    char bin_file[256];
    char dbg_file[300];
    int fd = mkstemps(asm_file, 4);
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
    int line_end_sequence_found = 0;
    int line_header_ok = 0;
    int debug_info_header_ok = 0;
    int debug_info_cu_abbrev_ok = 0;
    int debug_info_children_end_ok = 0;
    int subprogram_die_count = 0;
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
            uint16_t version = (uint16_t)info[4] | ((uint16_t)info[5] << 8);
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
                    p += cu_attr_size;

                    while (p < info_len) {
                        uint8_t abbrev_code = info[p++];
                        if (abbrev_code == 0) {
                            debug_info_children_end_ok = 1;
                            break;
                        }

                        if (abbrev_code != 2) {
                            break;
                        }

                        /* subprogram DIE: name(strp), decl_file(data1), decl_line(uleb),
                         * decl_col(uleb), external(flag), low_pc(addr), high_pc(data8) */
                        if (p + 5 > info_len) break;
                        p += 4; /* name */
                        p += 1; /* decl_file */

                        uint64_t uleb_val = 0;
                        p = decode_uleb128_at(info, info_len, p, &uleb_val);
                        if (p == (size_t)-1) break;
                        p = decode_uleb128_at(info, info_len, p, &uleb_val);
                        if (p == (size_t)-1) break;

                        if (p + 17 > info_len) break;
                        p += 1;  /* external */
                        p += 8;  /* low_pc */
                        p += 8;  /* high_pc */

                        subprogram_die_count++;
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
                uint16_t line_version = (uint16_t)line[4] | ((uint16_t)line[5] << 8);
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
    ASSERT_TRUE(subprogram_die_count >= 2);

    free(bytes);
    unlink(asm_file);
    unlink(bin_file);
    unlink(dbg_file);
    return 0;
}

/* Test: Validate DWARF output using readelf debug dump when tool is available */
int test_integration_dwarf_readelf_validation(void) {
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    call helper\n"
        "    mov rax, $60\n"
        "    mov rdi, $0\n"
        "    syscall\n"
        "helper:\n"
        "    ret\n";
    char asm_file[] = "/tmp/test_dbg_readelf_XXXXXX.asm";
    char bin_file[256];
    int fd = mkstemps(asm_file, 4);
    char cmd[512];
    char line[512];
    int saw_comp_dir = 0;
    int saw_subprogram = 0;
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
    ASSERT_TRUE(saw_start);
    ASSERT_TRUE(saw_line_section);
    ASSERT_TRUE(saw_file_name);

    unlink(asm_file);
    unlink(bin_file);
    return 0;
}

/* Test: DWARF generation should fail gracefully when debug buffers are exceeded */
int test_integration_dwarf_overflow_failure(void) {
    char asm_file[] = "/tmp/test_dbg_ovf_XXXXXX.asm";
    char bin_file[256];
    int fd = mkstemps(asm_file, 4);
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

/* Test Suite: Integration Tests */
TEST_SUITE(integration) {
    TEST(integration_exit);
    TEST(integration_arithmetic);
    TEST(integration_registers);
    TEST(integration_conditional);
    TEST(integration_loop);
    TEST(integration_pushpop);
    TEST(integration_callret);
    TEST(integration_8bit);
    TEST(integration_data);
    TEST(integration_empty);
    TEST(integration_invalid);
    TEST(integration_sectionless_entrypoint);
    TEST(integration_include_relative);
    TEST(integration_sse_smoke);
    TEST(integration_sse_mixed_controlflow);
    TEST(integration_high8_rex_conflict);
    TEST(integration_dwarf_sections);
    TEST(integration_dwarf_readelf_validation);
    TEST(integration_dwarf_overflow_failure);
}

/* Main entry point */
int main(void) {
    test_init();
    RUN_TEST_SUITE(integration);
    test_report();
    return test_final_status();
}
