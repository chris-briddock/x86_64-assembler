/**
 * Disassembler Unit Tests
 */

#include "test_framework.h"
#include "x86_64_asm/x86_64_asm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *run_disassemble_to_string(const uint8_t *code, size_t size, uint64_t base) {
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

static int test_disassemble_mov_xor_syscall(void) {
    static const uint8_t code[] = {
        0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,
        0x48, 0x31, 0xff,
        0x0f, 0x05
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x400000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "mov rax, 0x3c");
    ASSERT_STR_CONTAINS(out, "xor rdi, rdi");
    ASSERT_STR_CONTAINS(out, "syscall");

    free(out);
    return 0;
}

static int test_disassemble_call_ret_jmp_relative(void) {
    static const uint8_t code[] = {
        0xe8, 0x05, 0x00, 0x00, 0x00,
        0xc3,
        0xe9, 0xf5, 0xff, 0xff, 0xff
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x400000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "call 0x40000a");
    ASSERT_STR_CONTAINS(out, "ret");
    ASSERT_STR_CONTAINS(out, "jmp 0x400000");

    free(out);
    return 0;
}

static int test_disassemble_sib_memory_operand(void) {
    static const uint8_t code[] = {
        0x48, 0x8b, 0x44, 0x8b, 0x10
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x400010U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "mov rax, [rbx+rcx*4+0x10]");

    free(out);
    return 0;
}

static int test_disassemble_unknown_falls_back_to_db(void) {
    static const uint8_t code[] = {
        0x6c
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x500000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "db 0x6c");

    free(out);
    return 0;
}

static int test_disassemble_push_pop_and_nop_leave(void) {
    static const uint8_t code[] = {
        0x41, 0x57,
        0x41, 0x5f,
        0x6a, 0x08,
        0x68, 0x78, 0x56, 0x34, 0x12,
        0x90,
        0xc9
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x401000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "push r15");
    ASSERT_STR_CONTAINS(out, "pop r15");
    ASSERT_STR_CONTAINS(out, "push 0x8");
    ASSERT_STR_CONTAINS(out, "push 0x12345678");
    ASSERT_STR_CONTAINS(out, "nop");
    ASSERT_STR_CONTAINS(out, "leave");

    free(out);
    return 0;
}

static int test_disassemble_short_jumps_and_jcc(void) {
    static const uint8_t code[] = {
        0xeb, 0x02,
        0x90,
        0x90,
        0x74, 0x02,
        0x90,
        0x90
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x402000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "jmp 0x402004");
    ASSERT_STR_CONTAINS(out, "je 0x402008");

    free(out);
    return 0;
}

static int test_disassemble_lea_and_movsxd(void) {
    static const uint8_t code[] = {
        0x48, 0x8d, 0x4c, 0x8a, 0x20,
        0x48, 0x63, 0xc1,
        0x48, 0x63, 0x45, 0xf0
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x403000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "lea rcx, [rdx+rcx*4+0x20]");
    ASSERT_STR_CONTAINS(out, "movsxd rax, ecx");
    ASSERT_STR_CONTAINS(out, "movsxd rax, [rbp-0x10]");

    free(out);
    return 0;
}

static int test_disassemble_movzx_setcc_and_cmovcc(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xb6, 0x43, 0x10,
        0x0f, 0x95, 0xc0,
        0x0f, 0x9f, 0x45, 0xff,
        0x48, 0x0f, 0x45, 0xd8,
        0x48, 0x0f, 0x4e, 0x4e, 0x08
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x404000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "movzx rax, [rbx+0x10]");
    ASSERT_STR_CONTAINS(out, "setne al");
    ASSERT_STR_CONTAINS(out, "setg [rbp-0x1]");
    ASSERT_STR_CONTAINS(out, "cmovne rbx, rax");
    ASSERT_STR_CONTAINS(out, "cmovle rcx, [rsi+0x8]");

    free(out);
    return 0;
}

static int test_disassemble_movsx_imul_and_bit_tests(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xbe, 0x43, 0x01,
        0x48, 0x0f, 0xbf, 0x4d, 0xf8,
        0x48, 0x0f, 0xaf, 0xd8,
        0x48, 0x0f, 0xa3, 0xca,
        0x48, 0x0f, 0xab, 0x45, 0xf8,
        0x48, 0x0f, 0xb3, 0x4e, 0x10,
        0x48, 0x0f, 0xbb, 0xd1
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x405000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "movsx rax, [rbx+0x1]");
    ASSERT_STR_CONTAINS(out, "movsx rcx, [rbp-0x8]");
    ASSERT_STR_CONTAINS(out, "imul rbx, rax");
    ASSERT_STR_CONTAINS(out, "bt rdx, rcx");
    ASSERT_STR_CONTAINS(out, "bts [rbp-0x8], rax");
    ASSERT_STR_CONTAINS(out, "btr [rsi+0x10], rcx");
    ASSERT_STR_CONTAINS(out, "btc rcx, rdx");

    free(out);
    return 0;
}

static int test_disassemble_operand_override_16bit_forms(void) {
    static const uint8_t code[] = {
        0x66, 0x0f, 0xbe, 0xc1,
        0x66, 0x0f, 0xbf, 0xc2,
        0x66, 0x0f, 0xaf, 0xc1,
        0x66, 0x0f, 0xa3, 0xc1,
        0x66, 0x0f, 0xab, 0xc1,
        0x66, 0xb8, 0x34, 0x12,
        0x66, 0x81, 0xc1, 0x78, 0x56,
        0x66, 0xc7, 0xc2, 0xef, 0xcd
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x406000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "movsx ax, cl");
    ASSERT_STR_CONTAINS(out, "movsx ax, dx");
    ASSERT_STR_CONTAINS(out, "imul ax, cx");
    ASSERT_STR_CONTAINS(out, "bt cx, ax");
    ASSERT_STR_CONTAINS(out, "bts cx, ax");
    ASSERT_STR_CONTAINS(out, "mov ax, 0x1234");
    ASSERT_STR_CONTAINS(out, "add cx, 0x5678");
    ASSERT_STR_CONTAINS(out, "mov dx, 0xcdef");

    free(out);
    return 0;
}

static int test_disassemble_bit_test_immediate_forms(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xba, 0xe0, 0x05,
        0x48, 0x0f, 0xba, 0xe9, 0x07,
        0x48, 0x0f, 0xba, 0xf2, 0x03,
        0x48, 0x0f, 0xba, 0xfb, 0x01,
        0x48, 0x0f, 0xba, 0x6d, 0xf8, 0x02,
        0x66, 0x0f, 0xba, 0xe1, 0x04
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x407000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "bt rax, 0x5");
    ASSERT_STR_CONTAINS(out, "bts rcx, 0x7");
    ASSERT_STR_CONTAINS(out, "btr rdx, 0x3");
    ASSERT_STR_CONTAINS(out, "btc rbx, 0x1");
    ASSERT_STR_CONTAINS(out, "bts [rbp-0x8], 0x2");
    ASSERT_STR_CONTAINS(out, "bt cx, 0x4");

    free(out);
    return 0;
}

static int test_disassemble_bsf_bsr_and_test_forms(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xbc, 0xc3,
        0x48, 0x0f, 0xbd, 0x45, 0xf8,
        0x48, 0x85, 0xc8,
        0x48, 0x85, 0x4d, 0xf0,
        0x66, 0x0f, 0xbc, 0xc1,
        0x66, 0x0f, 0xbd, 0xc2,
        0x66, 0x85, 0xc8
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x408000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "bsf rax, rbx");
    ASSERT_STR_CONTAINS(out, "bsr rax, [rbp-0x8]");
    ASSERT_STR_CONTAINS(out, "test rax, rcx");
    ASSERT_STR_CONTAINS(out, "test [rbp-0x10], rcx");
    ASSERT_STR_CONTAINS(out, "bsf ax, cx");
    ASSERT_STR_CONTAINS(out, "bsr ax, dx");
    ASSERT_STR_CONTAINS(out, "test ax, cx");

    free(out);
    return 0;
}

static int test_disassemble_xadd_and_cmpxchg_forms(void) {
    static const uint8_t code[] = {
        0x48, 0x0f, 0xc1, 0xd8,
        0x0f, 0xc0, 0xc1,
        0x48, 0x0f, 0xb1, 0xd1,
        0x66, 0x0f, 0xb1, 0xc8,
        0x48, 0x0f, 0xc1, 0x8d, 0xf0, 0xff, 0xff, 0xff,
        0x0f, 0xb0, 0x45, 0xff
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x409000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "xadd rax, rbx");
    ASSERT_STR_CONTAINS(out, "xadd cl, al");
    ASSERT_STR_CONTAINS(out, "cmpxchg rcx, rdx");
    ASSERT_STR_CONTAINS(out, "cmpxchg ax, cx");
    ASSERT_STR_CONTAINS(out, "xadd [rbp-0x10], rcx");
    ASSERT_STR_CONTAINS(out, "cmpxchg [rbp-0x1], al");

    free(out);
    return 0;
}

static int test_disassemble_setcc_and_movx_edge_forms(void) {
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
    char *out = run_disassemble_to_string(code, sizeof(code), 0x40a000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "sete ah");
    ASSERT_STR_CONTAINS(out, "sete spl");
    ASSERT_STR_CONTAINS(out, "movzx r8d, spl");
    ASSERT_STR_CONTAINS(out, "movsx eax, ah");
    ASSERT_STR_CONTAINS(out, "movsx eax, spl");
    ASSERT_STR_CONTAINS(out, "movzx ax, cx");
    ASSERT_STR_CONTAINS(out, "setne [rip+0x1234]");
    ASSERT_STR_CONTAINS(out, "movzx rcx, [rip+0x10]");

    free(out);
    return 0;
}

static int test_disassemble_multibyte_nop_and_group80(void) {
    static const uint8_t code[] = {
        0x0f, 0x1f, 0x00,
        0x66, 0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0xc0, 0x01,
        0x80, 0xe1, 0x7f,
        0x80, 0x6d, 0xf8, 0x80,
        0x80, 0x3d, 0x10, 0x00, 0x00, 0x00, 0xff
    };
    char *out = run_disassemble_to_string(code, sizeof(code), 0x40b000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "nop [rax]");
    ASSERT_STR_CONTAINS(out, "nop [rax+rax*1+0x0]");
    ASSERT_STR_CONTAINS(out, "add al, 0x1");
    ASSERT_STR_CONTAINS(out, "and cl, 0x7f");
    ASSERT_STR_CONTAINS(out, "sub [rbp-0x8], 0x80");
    ASSERT_STR_CONTAINS(out, "cmp [rip+0x10], 0xff");

    free(out);
    return 0;
}

static int test_disassemble_group2_group3_and_byte_mov_forms(void) {
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
    char *out = run_disassemble_to_string(code, sizeof(code), 0x40c000U);

    ASSERT_NOT_NULL(out);
    ASSERT_STR_CONTAINS(out, "mov al, cl");
    ASSERT_STR_CONTAINS(out, "mov bl, cl");
    ASSERT_STR_CONTAINS(out, "mov spl, r8b");
    ASSERT_STR_CONTAINS(out, "shr al, 0x4");
    ASSERT_STR_CONTAINS(out, "shr rcx, 0x8");
    ASSERT_STR_CONTAINS(out, "sar cx, 0x1");
    ASSERT_STR_CONTAINS(out, "test cl, 0xf");
    ASSERT_STR_CONTAINS(out, "not al");
    ASSERT_STR_CONTAINS(out, "neg al");
    ASSERT_STR_CONTAINS(out, "test ecx, 0x1234");
    ASSERT_STR_CONTAINS(out, "test cx, 0xabcd");
    ASSERT_STR_CONTAINS(out, "not ecx");
    ASSERT_STR_CONTAINS(out, "neg ecx");

    free(out);
    return 0;
}

TEST_SUITE(disassembler) {
    TEST(disassemble_mov_xor_syscall);
    TEST(disassemble_call_ret_jmp_relative);
    TEST(disassemble_sib_memory_operand);
    TEST(disassemble_unknown_falls_back_to_db);
    TEST(disassemble_push_pop_and_nop_leave);
    TEST(disassemble_short_jumps_and_jcc);
    TEST(disassemble_lea_and_movsxd);
    TEST(disassemble_movzx_setcc_and_cmovcc);
    TEST(disassemble_movsx_imul_and_bit_tests);
    TEST(disassemble_operand_override_16bit_forms);
    TEST(disassemble_bit_test_immediate_forms);
    TEST(disassemble_bsf_bsr_and_test_forms);
    TEST(disassemble_xadd_and_cmpxchg_forms);
    TEST(disassemble_setcc_and_movx_edge_forms);
    TEST(disassemble_multibyte_nop_and_group80);
    TEST(disassemble_group2_group3_and_byte_mov_forms);
}

int main(void) {
    test_init();
    RUN_TEST_SUITE(disassembler);
    test_report();
    return test_final_status();
}
