/**
 * Parser Unit Tests
 * Tests the tokenizer and parser
 */

#include "test_framework.h"
#include "../src/x86_64_asm.h"
#include <string.h>

/* External parser functions from x86_64_asm.c */
extern parsed_instruction_t *parse_source(const char *source, int *count);
extern parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count);
extern void free_instructions(parsed_instruction_t *insts);
extern char *preprocess_macros(assembler_context_t *ctx, const char *source);

/* Test: Parse simple mov instruction */
int test_parse_mov(void) {
    int count = 0;
    const char *source = "mov rax, rbx\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(2, insts[0].operand_count);
    ASSERT_EQ(OPERAND_REG, insts[0].operands[0].type);
    ASSERT_EQ(OPERAND_REG, insts[0].operands[1].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[0].operands[1].reg);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse immediate value */
int test_parse_immediate(void) {
    int count = 0;
    const char *source = "mov rax, $42\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(42, insts[0].operands[1].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse hex immediate */
int test_parse_hex_immediate(void) {
    int count = 0;
    const char *source = "mov rax, $0xDEADBEEF\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(0xDEADBEEF, insts[0].operands[1].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse label reference */
int test_parse_label_ref(void) {
    int count = 0;
    const char *source = "jmp loop\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_JMP, insts[0].type);
    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[0].type);
    ASSERT_EQ_STR("loop", insts[0].operands[0].label);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse multiple instructions */
int test_parse_multiple(void) {
    int count = 0;
    const char *source = 
        "mov rax, $1\n"
        "mov rdi, $2\n"
        "syscall\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(3, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(INST_MOV, insts[1].type);
    ASSERT_EQ(INST_SYSCALL, insts[2].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse instruction with label */
int test_parse_labeled_instruction(void) {
    int count = 0;
    const char *source = "loop:\n  dec rcx\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    /* Label-only line generates empty instruction, then dec rcx */
    ASSERT_EQ(2, count);
    /* First instruction is empty (just label) */
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("loop", insts[0].label);
    /* Second instruction is dec rcx */
    ASSERT_EQ(INST_DEC, insts[1].type);
    ASSERT_EQ(RCX, insts[1].operands[0].reg);
    
    free_instructions(insts);
    return 0;
}

/* Test: Label column should reflect source indentation */
int test_parse_label_column(void) {
    int count = 0;
    const char *source = "    helper:\nret\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("helper", insts[0].label);
    ASSERT_EQ(5, insts[0].label_column);

    free_instructions(insts);
    return 0;
}

/* Test: Parse memory operand */
int test_parse_memory(void) {
    int count = 0;
    const char *source = "mov rax, [rbx]\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_EQ(RBX, insts[0].operands[1].mem.base);
    ASSERT_TRUE(insts[0].operands[1].mem.has_base);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse memory with displacement */
int test_parse_memory_disp(void) {
    int count = 0;
    const char *source = "mov rax, [rsp + 8]\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_EQ(RSP, insts[0].operands[1].mem.base);
    ASSERT_TRUE(insts[0].operands[1].mem.has_displacement);
    ASSERT_EQ(8, insts[0].operands[1].mem.displacement);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse indexed memory with scale factor 2 and displacement */
int test_parse_memory_index_scale2(void) {
    int count = 0;
    const char *source = "mov rax, [r12 + r13 * 2 + 16]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_TRUE(insts[0].operands[1].mem.has_base);
    ASSERT_EQ(R12, insts[0].operands[1].mem.base);
    ASSERT_TRUE(insts[0].operands[1].mem.has_index);
    ASSERT_EQ(R13, insts[0].operands[1].mem.index);
    ASSERT_EQ(2, insts[0].operands[1].mem.scale);
    ASSERT_TRUE(insts[0].operands[1].mem.has_displacement);
    ASSERT_EQ(16, insts[0].operands[1].mem.displacement);

    free_instructions(insts);
    return 0;
}

/* Test: Parse extended-register base+index memory operand with implicit scale 1 */
int test_parse_memory_ext_base_index(void) {
    int count = 0;
    const char *source = "mov rax, [r8 + r15]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_TRUE(insts[0].operands[1].mem.has_base);
    ASSERT_EQ(R8, insts[0].operands[1].mem.base);
    ASSERT_TRUE(insts[0].operands[1].mem.has_index);
    ASSERT_EQ(R15, insts[0].operands[1].mem.index);
    ASSERT_EQ(1, insts[0].operands[1].mem.scale);

    free_instructions(insts);
    return 0;
}

/* Test: Parse all registers */
int test_parse_registers(void) {
    int count = 0;
    const char *source = 
        "mov rax, rbx\n"
        "mov rcx, rdx\n"
        "mov rsi, rdi\n"
        "mov r8, r9\n"
        "mov r10, r11\n"
        "mov r12, r13\n"
        "mov r14, r15\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(7, count);
    
    /* Verify all registers parsed correctly */
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[0].operands[1].reg);
    ASSERT_EQ(R8, insts[3].operands[0].reg);
    ASSERT_EQ(R9, insts[3].operands[1].reg);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse section directive */
int test_parse_section(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "section .text\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    /* Section directives currently generate placeholder instructions */
    /* Just verify it parses without crashing */
    ASSERT_NOT_NULL(insts);
    ASSERT_TRUE(count >= 0);
    
    if (insts) free_instructions(insts);
    return 0;
}

/* Test: Parse empty source */
int test_parse_empty(void) {
    int count = 0;
    const char *source = "";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_EQ(0, count);
    
    if (insts) free_instructions(insts);
    return 0;
}

/* Test: Parse comment-only source */
int test_parse_comments(void) {
    int count = 0;
    const char *source = 
        "; This is a comment\n"
        "  ; Another comment\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_EQ(0, count);
    
    if (insts) free_instructions(insts);
    return 0;
}

/* Test: Parse instruction with comment */
int test_parse_instruction_with_comment(void) {
    int count = 0;
    const char *source = "mov rax, rbx  ; This is a comment\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse push/pop */
int test_parse_push_pop(void) {
    int count = 0;
    const char *source = 
        "push rax\n"
        "pop rbx\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_PUSH, insts[0].type);
    ASSERT_EQ(INST_POP, insts[1].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[1].operands[0].reg);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse call and ret */
int test_parse_call_ret(void) {
    int count = 0;
    const char *source =
        "call function\n"
        "ret\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_CALL, insts[0].type);
    ASSERT_EQ(INST_RET, insts[1].type);
    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[0].type);
    ASSERT_EQ_STR("function", insts[0].operands[0].label);
    
    free_instructions(insts);
    return 0;
}

/* Regression test: Unknown instruction should fail parsing */
int test_parse_unknown_instruction(void) {
    int count = 0;
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    invalid_instruction\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);

    return 0;
}

/* Test: Parser should reject SSE move memory destination with non-XMM source */
int test_parse_sse_move_mem_dst_imm_src_invalid(void) {
    int count = 0;
    char *captured_err;
    const char *source = "movups [rax], $1\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "memory destination requires XMM source");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject SSE arithmetic with non-XMM destination */
int test_parse_sse_arith_non_xmm_dst_invalid(void) {
    int count = 0;
    char *captured_err;
    const char *source = "addss rax, xmm1\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "destination must be XMM register");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject SSE arithmetic with immediate source */
int test_parse_sse_arith_imm_src_invalid(void) {
    int count = 0;
    char *captured_err;
    const char *source = "divps xmm3, $1\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "source must be XMM or memory");

    free(captured_err);
    return 0;
}

/* Test: Macro definition and lookup */
int test_macro_definition(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro exit_code code\n"
        "    mov rax, 60\n"
        "    mov rdi, \\code\n"
        "    syscall\n"
        ".endm\n";
    
    char *preprocessed = preprocess_macros(ctx, source);
    ASSERT_NOT_NULL(preprocessed);
    
    /* Check that macro was defined */
    ASSERT_EQ(1, ctx->macro_count);
    ASSERT_EQ_STR("exit_code", ctx->macros[0].name);
    ASSERT_EQ(1, ctx->macros[0].param_count);
    ASSERT_EQ_STR("code", ctx->macros[0].params[0].name);
    ASSERT_EQ(3, ctx->macros[0].body_line_count);
    
    free(preprocessed);
    asm_free(ctx);
    return 0;
}

/* Test: Macro expansion with parameter substitution */
int test_macro_expansion(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro set_reg reg, val\n"
        "    mov \\reg, \\val\n"
        ".endm\n"
        "set_reg rax, 42\n";
    
    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(2, insts[0].operand_count);
    ASSERT_EQ(OPERAND_REG, insts[0].operands[0].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(42, insts[0].operands[1].immediate);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Macro with multiple parameters */
int test_macro_multiple_params(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro add_regs dst, src1, src2\n"
        "    mov \\dst, \\src1\n"
        "    add \\dst, \\src2\n"
        ".endm\n"
        "add_regs rax, rbx, rcx\n";
    
    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    
    /* First instruction: mov rax, rbx */
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[0].operands[1].reg);
    
    /* Second instruction: add rax, rcx */
    ASSERT_EQ(INST_ADD, insts[1].type);
    ASSERT_EQ(RAX, insts[1].operands[0].reg);
    ASSERT_EQ(RCX, insts[1].operands[1].reg);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Macro with alternative $param syntax */
int test_macro_dollar_syntax(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro load_imm reg, val\n"
        "    mov $reg, $val\n"
        ".endm\n"
        "load_imm rax, 100\n";
    
    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(100, insts[0].operands[1].immediate);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Macro without parameters */
int test_macro_no_params(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro push_all\n"
        "    push rax\n"
        "    push rbx\n"
        ".endm\n"
        "push_all\n";
    
    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_PUSH, insts[0].type);
    ASSERT_EQ(INST_PUSH, insts[1].type);
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[1].operands[0].reg);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Multiple macro invocations */
int test_macro_multiple_invocations(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source =
        ".macro zero_reg reg\n"
        "    xor \\reg, \\reg\n"
        ".endm\n"
        "zero_reg rax\n"
        "zero_reg rbx\n"
        "zero_reg rcx\n";
    
    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(3, count);
    
    ASSERT_EQ(RAX, insts[0].operands[0].reg);
    ASSERT_EQ(RBX, insts[1].operands[0].reg);
    ASSERT_EQ(RCX, insts[2].operands[0].reg);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test Suite: Parser Tests */
TEST_SUITE(parser) {
    TEST(parse_mov);
    TEST(parse_immediate);
    TEST(parse_hex_immediate);
    TEST(parse_label_ref);
    TEST(parse_multiple);
    TEST(parse_labeled_instruction);
    TEST(parse_label_column);
    TEST(parse_memory);
    TEST(parse_memory_disp);
    TEST(parse_memory_index_scale2);
    TEST(parse_memory_ext_base_index);
    TEST(parse_registers);
    TEST(parse_section);
    TEST(parse_empty);
    TEST(parse_comments);
    TEST(parse_instruction_with_comment);
    TEST(parse_push_pop);
    TEST(parse_call_ret);
    TEST(parse_unknown_instruction);
    TEST(parse_sse_move_mem_dst_imm_src_invalid);
    TEST(parse_sse_arith_non_xmm_dst_invalid);
    TEST(parse_sse_arith_imm_src_invalid);
    TEST(macro_definition);
    TEST(macro_expansion);
    TEST(macro_multiple_params);
    TEST(macro_dollar_syntax);
    TEST(macro_no_params);
    TEST(macro_multiple_invocations);
}

/* Main entry point */
int main(void) {
    test_init();
    RUN_TEST_SUITE(parser);
    test_report();
    return test_final_status();
}
