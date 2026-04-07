/**
 * Parser Unit Tests
 * Tests the tokenizer and parser
 */

#include "test_framework.h"
#include "x86_64_asm/x86_64_asm.h"
#include <string.h>

/* External parser functions from x86_64_asm.c */
extern parsed_instruction_t *parse_source(const char *source, int *count);
extern parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count);
extern void free_instructions(parsed_instruction_t *insts);
extern char *preprocess_macros(assembler_context_t *ctx, const char *source);

/* Test: Parse simple mov instruction */
static int test_parse_mov(void) {
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
static int test_parse_immediate(void) {
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
static int test_parse_hex_immediate(void) {
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
static int test_parse_label_ref(void) {
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

/* Test: Parse local and anonymous label syntax */
static int test_parse_local_and_anonymous_labels(void) {
    int count = 0;
    const char *source =
        "entry:\n"
        "    jmp .loop\n"
        ".loop:\n"
        "    jmp @F\n"
        "@@:\n"
        "    jmp @B\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(6, count);

    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("entry", insts[0].label);

    ASSERT_EQ(INST_JMP, insts[1].type);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR(".loop", insts[1].operands[0].label);

    ASSERT_TRUE(insts[2].has_label);
    ASSERT_EQ_STR(".loop", insts[2].label);

    ASSERT_EQ(INST_JMP, insts[3].type);
    ASSERT_EQ_STR("@F", insts[3].operands[0].label);

    ASSERT_TRUE(insts[4].has_label);
    ASSERT_EQ_STR("@@", insts[4].label);

    ASSERT_EQ(INST_JMP, insts[5].type);
    ASSERT_EQ_STR("@B", insts[5].operands[0].label);

    free_instructions(insts);
    return 0;
}

/* Test: Parse multiple instructions */
static int test_parse_multiple(void) {
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
static int test_parse_labeled_instruction(void) {
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
static int test_parse_label_column(void) {
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
static int test_parse_memory(void) {
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
static int test_parse_memory_disp(void) {
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

/* Test: Parse memory operands with int32 displacement boundaries */
static int test_parse_memory_displacement_extremes(void) {
    int count = 0;
    const char *source =
        "mov rax, [rbx + 2147483647]\n"
        "mov rcx, [rdx - 2147483648]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);

    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_TRUE(insts[0].operands[1].mem.has_displacement);
    ASSERT_EQ(2147483647, insts[0].operands[1].mem.displacement);

    ASSERT_EQ(OPERAND_MEM, insts[1].operands[1].type);
    ASSERT_TRUE(insts[1].operands[1].mem.has_displacement);
    ASSERT_EQ(-2147483648LL, insts[1].operands[1].mem.displacement);

    free_instructions(insts);
    return 0;
}

/* Test: Parse near-maximum label length without truncation */
static int test_parse_label_max_length(void) {
    int count = 0;
    char label[MAX_LABEL_LENGTH];
    char source[600];

    memset(label, 'a', MAX_LABEL_LENGTH - 1);
    label[MAX_LABEL_LENGTH - 1] = '\0';

    source[0] = '\0';
    strcat(source, label);
    strcat(source, ":\n");
    strcat(source, "    nop\n");

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR(label, insts[0].label);
    ASSERT_EQ(INST_NOP, insts[1].type);

    free_instructions(insts);
    return 0;
}

/* Test: Parse indexed memory with scale factor 2 and displacement */
static int test_parse_memory_index_scale2(void) {
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
static int test_parse_memory_ext_base_index(void) {
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
static int test_parse_registers(void) {
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
static int test_parse_section(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "section .text\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_SECTION, insts[1].type);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[0].type);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR(".data", insts[0].operands[0].label);
    ASSERT_EQ_STR(".text", insts[1].operands[0].label);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse named section directives and segment alias */
static int test_parse_named_sections_and_segment_alias(void) {
    int count = 0;
    const char *source =
        "section .text.startup\n"
        "segment .data.rel.ro\n"
        ".bss\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(3, count);

    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_SECTION, insts[1].type);
    ASSERT_EQ(INST_SECTION, insts[2].type);

    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(1, insts[2].operand_count);

    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[0].type);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ(OPERAND_LABEL, insts[2].operands[0].type);

    ASSERT_EQ_STR(".text.startup", insts[0].operands[0].label);
    ASSERT_EQ_STR(".data.rel.ro", insts[1].operands[0].label);
    ASSERT_EQ_STR(".bss", insts[2].operands[0].label);

    free_instructions(insts);
    return 0;
}

/* Test: Parse empty source */
static int test_parse_empty(void) {
    int count = 0;
    const char *source = "";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_EQ(0, count);
    
    if (insts) free_instructions(insts);
    return 0;
}

/* Test: Parse comment-only source */
static int test_parse_comments(void) {
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
static int test_parse_instruction_with_comment(void) {
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
static int test_parse_push_pop(void) {
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
static int test_parse_call_ret(void) {
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

/* Test: Parse int immediate instruction */
static int test_parse_int(void) {
    int count = 0;
    const char *source = "int $0x80\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_INT, insts[0].type);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[0].type);
    ASSERT_EQ(0x80, insts[0].operands[0].immediate);

    free_instructions(insts);
    return 0;
}

/* Test: Parse enter/cbw/cwd/cwde mnemonics */
static int test_parse_enter_and_legacy_signext(void) {
    int count = 0;
    const char *source =
        "enter 16, 0\n"
        "cbw\n"
        "cwd\n"
        "cwde\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(4, count);

    ASSERT_EQ(INST_ENTER, insts[0].type);
    ASSERT_EQ(2, insts[0].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[0].type);
    ASSERT_EQ(16, insts[0].operands[0].immediate);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(0, insts[0].operands[1].immediate);

    ASSERT_EQ(INST_CBW, insts[1].type);
    ASSERT_EQ(INST_CWD, insts[2].type);
    ASSERT_EQ(INST_CWDE, insts[3].type);

    free_instructions(insts);
    return 0;
}

/* Regression test: Unknown instruction should fail parsing */
static int test_parse_unknown_instruction(void) {
    int count = 0;
    char *captured_err;
    const char *source =
        "section .text\n"
        "global _start\n"
        "_start:\n"
        "    invalid_instruction\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line ");
    ASSERT_STR_CONTAINS(captured_err, ", column ");
    ASSERT_STR_CONTAINS(captured_err, "[Syntax] Unknown instruction");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");
    ASSERT_STR_CONTAINS(captured_err, "^");

    free(captured_err);

    return 0;
}

/* Regression test: Unknown mnemonic typo should provide did-you-mean guidance */
static int test_parse_unknown_instruction_did_you_mean(void) {
    int count = 0;
    char *captured_err;
    const char *source = "ad rax, rbx\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Unknown instruction 'ad'");
    ASSERT_STR_CONTAINS(captured_err, "Did you mean 'add'");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Regression test: malformed memory operand should include concrete syntax guidance */
static int test_parse_memory_operand_suggestion(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, [rbx + ]\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Expected register or number after +/- in memory operand");
    ASSERT_STR_CONTAINS(captured_err, "After '+' or '-', use a register, numeric displacement, or label");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Regression test: segment override without memory operand should suggest valid form */
static int test_parse_segment_override_requires_memory(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, fs:rax\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Segment override requires a memory operand");
    ASSERT_STR_CONTAINS(captured_err, "fs:[rax]");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject SSE move memory destination with non-XMM source */
static int test_parse_sse_move_mem_dst_imm_src_invalid(void) {
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
static int test_parse_sse_arith_non_xmm_dst_invalid(void) {
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
static int test_parse_sse_arith_imm_src_invalid(void) {
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

/* Test: Parser should reject invalid scale factor 5 */
static int test_parse_memory_invalid_scale5(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, [rbx + rcx * 5]\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 1, column");
    ASSERT_STR_CONTAINS(captured_err, "[Syntax]");
    ASSERT_STR_CONTAINS(captured_err, "Expected scale factor (1, 2, 4, or 8)");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject invalid scale factor 6 */
static int test_parse_memory_invalid_scale6(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, [rbx + rcx * 6]\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Expected scale factor (1, 2, 4, or 8)");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject invalid scale factor 7 */
static int test_parse_memory_invalid_scale7(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, [rbx + rcx * 7]\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Expected scale factor (1, 2, 4, or 8)");

    free(captured_err);
    return 0;
}

/* Test: Parser should reject RIP-relative form that also includes index/base */
static int test_parse_rip_relative_with_index_invalid(void) {
    int count = 0;
    char *captured_err;
    const char *source = "mov rax, [rip + rcx * 2]\n";

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source(source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "RIP-relative addressing cannot combine base/index registers");

    free(captured_err);
    return 0;
}

/* Test: Parser should accept legal RIP-relative addressing without base/index */
static int test_parse_rip_relative_legal(void) {
    int count = 0;
    const char *source = "mov rax, [rip + 8]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_TRUE(insts[0].operands[1].mem.is_rip_relative);
    ASSERT_EQ(8, insts[0].operands[1].mem.displacement);
    ASSERT_FALSE(insts[0].operands[1].mem.has_base);
    ASSERT_FALSE(insts[0].operands[1].mem.has_index);

    free_instructions(insts);
    return 0;
}

/* Test: Parse explicit absolute memory addressing */
static int test_parse_absolute_memory_addressing(void) {
    int count = 0;
    const char *source = "mov rax, [abs 0x1234]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_TRUE(insts[0].operands[1].mem.is_absolute);
    ASSERT_TRUE(insts[0].operands[1].mem.has_displacement);
    ASSERT_EQ(0x1234, insts[0].operands[1].mem.displacement);
    ASSERT_FALSE(insts[0].operands[1].mem.is_rip_relative);

    free_instructions(insts);
    return 0;
}

/* Test: Parse FS/GS segment override memory operands */
static int test_parse_segment_override_memory(void) {
    int count = 0;
    const char *source =
        "mov rax, fs:[0]\n"
        "mov rbx, gs:[8]\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);

    ASSERT_EQ(OPERAND_MEM, insts[0].operands[1].type);
    ASSERT_EQ(MEM_SEG_FS, insts[0].operands[1].mem.segment_override);
    ASSERT_TRUE(insts[0].operands[1].mem.has_displacement);
    ASSERT_EQ(0, insts[0].operands[1].mem.displacement);

    ASSERT_EQ(OPERAND_MEM, insts[1].operands[1].type);
    ASSERT_EQ(MEM_SEG_GS, insts[1].operands[1].mem.segment_override);
    ASSERT_TRUE(insts[1].operands[1].mem.has_displacement);
    ASSERT_EQ(8, insts[1].operands[1].mem.displacement);

    free_instructions(insts);
    return 0;
}

/* Test: Macro definition and lookup */
static int test_macro_definition(void) {
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
static int test_macro_expansion(void) {
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
static int test_macro_multiple_params(void) {
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
static int test_macro_dollar_syntax(void) {
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
static int test_macro_no_params(void) {
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
static int test_macro_multiple_invocations(void) {
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

/* Test: Recursive macro invocation should fail with actionable diagnostics */
static int test_macro_recursive_invocation_fails(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        ".macro recur\n"
        "    recur\n"
        ".endm\n"
        "recur\n";

    char *captured_err;
    int count = 0;

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Unknown instruction 'recur'");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    asm_free(ctx);
    return 0;
}

/* Test: %define should substitute symbols in active lines */
static int test_preprocessor_define_substitution(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        "%define EXIT_CODE 42\n"
        "mov rdi, EXIT_CODE\n";

    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(42, insts[0].operands[1].immediate);

    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: %if/%else/%endif should include only active branch */
static int test_preprocessor_if_else(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        "%define ENABLE_PATH 1\n"
        "%if ENABLE_PATH\n"
        "mov rax, 1\n"
        "%else\n"
        "mov rax, 2\n"
        "%endif\n";

    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(1, insts[0].operands[1].immediate);

    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: %ifdef/%ifndef should evaluate based on %define symbols */
static int test_preprocessor_ifdef_ifndef(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        "%define ENABLED 1\n"
        "%ifdef ENABLED\n"
        "mov rax, 7\n"
        "%endif\n"
        "%ifndef DISABLED\n"
        "mov rbx, 8\n"
        "%endif\n";

    int count = 0;
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(INST_MOV, insts[1].type);
    ASSERT_EQ(7, insts[0].operands[1].immediate);
    ASSERT_EQ(8, insts[1].operands[1].immediate);

    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: %warning should emit warning and continue preprocessing */
static int test_preprocessor_warning_directive(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        "%warning this is a warning\n"
        "mov rax, 3\n";
    char *captured_err;
    int count = 0;

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Warning: this is a warning");

    free(captured_err);
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: %error should fail preprocessing */
static int test_preprocessor_error_directive(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);

    const char *source =
        "%error hard stop\n"
        "mov rax, 1\n";
    char *captured_err;
    int count = 0;

    ASSERT_EQ(0, test_capture_stderr_begin());
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    captured_err = test_capture_stderr_end();

    ASSERT_NULL(insts);
    ASSERT_EQ(0, count);
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line");
    ASSERT_STR_CONTAINS(captured_err, "hard stop");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");
    ASSERT_STR_CONTAINS(captured_err, "Macro preprocessing failed");

    free(captured_err);
    asm_free(ctx);
    return 0;
}

/* Test: Parse times directive expansion */
static int test_parse_times_directive(void) {
    int count = 0;
    const char *source = "times 3 nop\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(3, count);
    ASSERT_EQ(INST_NOP, insts[0].type);
    ASSERT_EQ(INST_NOP, insts[1].type);
    ASSERT_EQ(INST_NOP, insts[2].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse times with larger count */
static int test_parse_times_large_count(void) {
    int count = 0;
    const char *source = "times 10 nop\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(10, count);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(INST_NOP, insts[i].type);
    }
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse equ directive */
static int test_parse_equ_directive(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source = "BUFFER_SIZE equ 256\n";
    int count = 0;
    
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_EQU, insts[0].type);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("BUFFER_SIZE", insts[0].label);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[0].type);
    ASSERT_EQ(256, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Parse equ with hex value */
static int test_parse_equ_hex(void) {
    assembler_context_t *ctx = asm_init();
    ASSERT_NOT_NULL(ctx);
    
    const char *source = "MAGIC equ 0x12345678\n";
    int count = 0;
    
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_EQU, insts[0].type);
    ASSERT_EQ(0x12345678, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    asm_free(ctx);
    return 0;
}

/* Test: Parse resb directive */
static int test_parse_resb(void) {
    int count = 0;
    const char *source = "buffer: resb 64\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_RESB, insts[0].type);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("buffer", insts[0].label);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[0].type);
    ASSERT_EQ(64, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse equ with label subtraction expression */
static int test_parse_equ_label_subtraction(void) {
    int count = 0;
    const char *source = "DELTA equ end - start\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_EQU, insts[0].type);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ_STR("DELTA", insts[0].label);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_LABEL_DIFF, insts[0].operands[0].type);
    ASSERT_EQ_STR("end", insts[0].operands[0].label);
    ASSERT_EQ_STR("start", insts[0].operands[0].label_rhs);

    free_instructions(insts);
    return 0;
}

/* Test: Parse weak/hidden symbol attribute directives */
static int test_parse_weak_hidden_directives(void) {
    int count = 0;
    const char *source =
        "weak foo, bar\n"
        ".hidden foo\n";

    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);

    ASSERT_EQ(INST_WEAK, insts[0].type);
    ASSERT_EQ(2, insts[0].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[0].type);
    ASSERT_EQ_STR("foo", insts[0].operands[0].label);
    ASSERT_EQ(OPERAND_LABEL, insts[0].operands[1].type);
    ASSERT_EQ_STR("bar", insts[0].operands[1].label);

    ASSERT_EQ(INST_HIDDEN, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR("foo", insts[1].operands[0].label);

    free_instructions(insts);
    return 0;
}

/* Test: Parse resw directive */
static int test_parse_resw(void) {
    int count = 0;
    const char *source = "words: resw 10\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_RESW, insts[0].type);
    ASSERT_EQ(10, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse resd directive */
static int test_parse_resd(void) {
    int count = 0;
    const char *source = "dwords: resd 5\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_RESD, insts[0].type);
    ASSERT_EQ(5, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse resq directive */
static int test_parse_resq(void) {
    int count = 0;
    const char *source = "qwords: resq 4\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_RESQ, insts[0].type);
    ASSERT_EQ(4, insts[0].operands[0].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse db with string literal */
static int test_parse_db_string(void) {
    int count = 0;
    const char *source = "msg: db \"Hello\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_DB, insts[0].type);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[0].operands[0].type);
    ASSERT_EQ_STR("Hello", insts[0].operands[0].string);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse db with string containing escape sequences */
static int test_parse_db_string_escapes(void) {
    int count = 0;
    const char *source = "msg: db \"Hello\\nWorld\\t!\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_DB, insts[0].type);
    ASSERT_EQ(OPERAND_STRING, insts[0].operands[0].type);
    /* The escape sequences should be processed: \n -> newline, \t -> tab */
    ASSERT_TRUE(strchr(insts[0].operands[0].string, '\n') != NULL);
    ASSERT_TRUE(strchr(insts[0].operands[0].string, '\t') != NULL);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse db with mixed string and immediates */
static int test_parse_db_mixed(void) {
    int count = 0;
    const char *source = "msg: db \"Hi\", 10, 0\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_DB, insts[0].type);
    ASSERT_EQ(3, insts[0].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[0].operands[0].type);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[2].type);
    ASSERT_EQ(10, insts[0].operands[1].immediate);
    ASSERT_EQ(0, insts[0].operands[2].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse .bss section directive */
static int test_parse_bss_section(void) {
    int count = 0;
    const char *source =
        "section .bss\n"
        "buffer: resb 64\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_RESB, insts[1].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse .rodata section directive */
static int test_parse_rodata_section(void) {
    int count = 0;
    const char *source =
        "section .rodata\n"
        "msg: db \"Hello\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_DB, insts[1].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse character literal */
static int test_parse_char_literal(void) {
    int count = 0;
    const char *source = "mov al, 'A'\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(2, insts[0].operand_count);
    ASSERT_EQ(OPERAND_REG, insts[0].operands[0].type);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(65, insts[0].operands[1].immediate);  /* 'A' = 65 */
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse character literal with escape sequence */
static int test_parse_char_literal_escape(void) {
    int count = 0;
    const char *source = "mov al, '\\n'\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_MOV, insts[0].type);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[1].type);
    ASSERT_EQ(10, insts[0].operands[1].immediate);  /* '\n' = 10 */
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse character literal in db directive */
static int test_parse_char_literal_in_db(void) {
    int count = 0;
    const char *source = "char_a: db 'A'\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(1, count);
    ASSERT_EQ(INST_DB, insts[0].type);
    ASSERT_TRUE(insts[0].has_label);
    ASSERT_EQ(1, insts[0].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[0].operands[0].type);
    ASSERT_EQ(65, insts[0].operands[0].immediate);  /* 'A' = 65 */
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse character literal in instruction */
static int test_parse_char_literal_in_instruction(void) {
    int count = 0;
    const char *source =
        "section .text\n"
        "mov al, 'A'\n"
        "mov bl, '\\t'\n"
        "mov cl, '\\0'\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(4, count);  /* section + 3 mov */
    ASSERT_EQ(INST_MOV, insts[1].type);
    ASSERT_EQ(INST_MOV, insts[2].type);
    ASSERT_EQ(INST_MOV, insts[3].type);
    ASSERT_EQ(65, insts[1].operands[1].immediate);   /* 'A' */
    ASSERT_EQ(9, insts[2].operands[1].immediate);    /* '\t' */
    ASSERT_EQ(0, insts[3].operands[1].immediate);    /* '\0' */
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse align directive */
static int test_parse_align(void) {
    int count = 0;
    const char *source =
        "section .text\n"
        "nop\n"
        "align 8\n"
        "nop\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(4, count);  /* section + nop + align + nop */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_NOP, insts[1].type);
    ASSERT_EQ(INST_ALIGN, insts[2].type);
    ASSERT_EQ(1, insts[2].operand_count);
    ASSERT_EQ(OPERAND_IMM, insts[2].operands[0].type);
    ASSERT_EQ(8, insts[2].operands[0].immediate);
    ASSERT_EQ(INST_NOP, insts[3].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse align directive with different alignments */
static int test_parse_align_various(void) {
    int count = 0;
    const char *source =
        "section .text\n"
        "align 4\n"
        "align 8\n"
        "align 16\n"
        "align 32\n"
        "align 64\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(6, count);  /* section + 5 align */
    ASSERT_EQ(INST_ALIGN, insts[1].type);
    ASSERT_EQ(4, insts[1].operands[0].immediate);
    ASSERT_EQ(INST_ALIGN, insts[2].type);
    ASSERT_EQ(8, insts[2].operands[0].immediate);
    ASSERT_EQ(INST_ALIGN, insts[3].type);
    ASSERT_EQ(16, insts[3].operands[0].immediate);
    ASSERT_EQ(INST_ALIGN, insts[4].type);
    ASSERT_EQ(32, insts[4].operands[0].immediate);
    ASSERT_EQ(INST_ALIGN, insts[5].type);
    ASSERT_EQ(64, insts[5].operands[0].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse align in data section */
static int test_parse_align_data(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db 0x01\n"
        "align 4\n"
        "dd 0x12345678\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(4, count);  /* section + db + align + dd */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(INST_ALIGN, insts[2].type);
    ASSERT_EQ(4, insts[2].operands[0].immediate);
    ASSERT_EQ(INST_DD, insts[3].type);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse UTF-8 string with emoji */
static int test_parse_utf8_string(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"Hello 🎉\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    /* UTF-8 emoji 🎉 = 0xF0 0x9F 0x8E 0x89 (4 bytes) + 5 ASCII chars + null */
    ASSERT_TRUE(strlen(insts[1].operands[0].string) >= 9);
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse UTF-8 string with Chinese characters */
static int test_parse_utf8_chinese(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"你好世界\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    /* Each Chinese char is 3 bytes in UTF-8, 4 chars = 12 bytes */
    ASSERT_EQ(12, (int)strlen(insts[1].operands[0].string));
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse UTF-8 string mixed with escapes */
static int test_parse_utf8_mixed(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"Text\\n你好\\tEnd\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    
    /* Verify the string contains the UTF-8 content and escape sequences */
    const char *str = insts[1].operands[0].string;
    /* String should be: "Text\n你好\tEnd" */
    ASSERT_TRUE(str[0] == 'T');
    ASSERT_TRUE(str[1] == 'e');
    ASSERT_TRUE(str[2] == 'x');
    ASSERT_TRUE(str[3] == 't');
    ASSERT_TRUE(str[4] == '\n');  /* Escaped newline */
    /* Then 6 bytes of UTF-8 Chinese characters */
    ASSERT_TRUE((unsigned char)str[5] == 0xE4);  /* First byte of 你 */
    ASSERT_TRUE(str[11] == '\t');  /* Escaped tab after Chinese chars */
    
    free_instructions(insts);
    return 0;
}

/* Test: Parse incbin directive */
static int test_parse_incbin(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "incbin \"test.bin\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + incbin */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_INCBIN, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    ASSERT_EQ_STR("test.bin", insts[1].operands[0].string);
    
    free_instructions(insts);
    return 0;
}

/* Test: String concatenation (two strings) */
static int test_parse_string_concat(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"Hello \" \"World\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    ASSERT_EQ_STR("Hello World", insts[1].operands[0].string);
    
    free_instructions(insts);
    return 0;
}

/* Test: String concatenation (multiple strings) */
static int test_parse_string_concat_multiple(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"A\" \"B\" \"C\" \"D\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    ASSERT_EQ_STR("ABCD", insts[1].operands[0].string);
    
    free_instructions(insts);
    return 0;
}

/* Test: String concatenation with escape sequences */
static int test_parse_string_concat_with_escapes(void) {
    int count = 0;
    const char *source =
        "section .data\n"
        "db \"Line1\\n\" \"Line2\\n\"\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + db */
    ASSERT_EQ(INST_DB, insts[1].type);
    ASSERT_EQ(1, insts[1].operand_count);
    ASSERT_EQ(OPERAND_STRING, insts[1].operands[0].type);
    /* Result should be "Line1\nLine2\n" with actual newline chars */
    const char *str = insts[1].operands[0].string;
    ASSERT_TRUE(str[5] == '\n');  /* First \n */
    ASSERT_TRUE(str[11] == '\n'); /* Second \n */
    
    free_instructions(insts);
    return 0;
}

/* Test: .comm directive */
static int test_parse_comm(void) {
    int count = 0;
    const char *source =
        "section .bss\n"
        ".comm buffer, 256\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + comm */
    ASSERT_EQ(INST_SECTION, insts[0].type);
    ASSERT_EQ(INST_COMM, insts[1].type);
    ASSERT_EQ(2, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR("buffer", insts[1].operands[0].label);
    ASSERT_EQ(OPERAND_IMM, insts[1].operands[1].type);
    ASSERT_EQ(256, insts[1].operands[1].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: .comm directive with alignment */
static int test_parse_comm_with_alignment(void) {
    int count = 0;
    const char *source =
        "section .bss\n"
        ".comm array, 1024, 8\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + comm */
    ASSERT_EQ(INST_COMM, insts[1].type);
    ASSERT_EQ(3, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR("array", insts[1].operands[0].label);
    ASSERT_EQ(1024, insts[1].operands[1].immediate);
    ASSERT_EQ(8, insts[1].operands[2].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: .lcomm directive */
static int test_parse_lcomm(void) {
    int count = 0;
    const char *source =
        "section .bss\n"
        ".lcomm local_buf, 128\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + lcomm */
    ASSERT_EQ(INST_LCOMM, insts[1].type);
    ASSERT_EQ(2, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR("local_buf", insts[1].operands[0].label);
    ASSERT_EQ(128, insts[1].operands[1].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test: .lcomm directive with alignment */
static int test_parse_lcomm_with_alignment(void) {
    int count = 0;
    const char *source =
        "section .bss\n"
        ".lcomm aligned_buf, 512, 16\n";
    
    parsed_instruction_t *insts = parse_source(source, &count);
    ASSERT_NOT_NULL(insts);
    ASSERT_EQ(2, count);  /* section + lcomm */
    ASSERT_EQ(INST_LCOMM, insts[1].type);
    ASSERT_EQ(3, insts[1].operand_count);
    ASSERT_EQ(OPERAND_LABEL, insts[1].operands[0].type);
    ASSERT_EQ_STR("aligned_buf", insts[1].operands[0].label);
    ASSERT_EQ(512, insts[1].operands[1].immediate);
    ASSERT_EQ(16, insts[1].operands[2].immediate);
    
    free_instructions(insts);
    return 0;
}

/* Test Suite: Parser Tests */
TEST_SUITE(parser) {
    TEST(parse_mov);
    TEST(parse_immediate);
    TEST(parse_hex_immediate);
    TEST(parse_label_ref);
    TEST(parse_local_and_anonymous_labels);
    TEST(parse_multiple);
    TEST(parse_labeled_instruction);
    TEST(parse_label_column);
    TEST(parse_memory);
    TEST(parse_memory_disp);
    TEST(parse_memory_displacement_extremes);
    TEST(parse_label_max_length);
    TEST(parse_memory_index_scale2);
    TEST(parse_memory_ext_base_index);
    TEST(parse_registers);
    TEST(parse_section);
    TEST(parse_named_sections_and_segment_alias);
    TEST(parse_empty);
    TEST(parse_comments);
    TEST(parse_instruction_with_comment);
    TEST(parse_push_pop);
    TEST(parse_call_ret);
    TEST(parse_int);
    TEST(parse_enter_and_legacy_signext);
    TEST(parse_unknown_instruction);
    TEST(parse_unknown_instruction_did_you_mean);
    TEST(parse_memory_operand_suggestion);
    TEST(parse_segment_override_requires_memory);
    TEST(parse_sse_move_mem_dst_imm_src_invalid);
    TEST(parse_sse_arith_non_xmm_dst_invalid);
    TEST(parse_sse_arith_imm_src_invalid);
    TEST(parse_memory_invalid_scale5);
    TEST(parse_memory_invalid_scale6);
    TEST(parse_memory_invalid_scale7);
    TEST(parse_rip_relative_with_index_invalid);
    TEST(parse_rip_relative_legal);
    TEST(parse_absolute_memory_addressing);
    TEST(parse_segment_override_memory);
    TEST(macro_definition);
    TEST(macro_expansion);
    TEST(macro_multiple_params);
    TEST(macro_dollar_syntax);
    TEST(macro_no_params);
    TEST(macro_multiple_invocations);
    TEST(macro_recursive_invocation_fails);
    TEST(preprocessor_define_substitution);
    TEST(preprocessor_if_else);
    TEST(preprocessor_ifdef_ifndef);
    TEST(preprocessor_warning_directive);
    TEST(preprocessor_error_directive);

    /* New directive tests */
    TEST(parse_times_directive);
    TEST(parse_times_large_count);
    TEST(parse_equ_directive);
    TEST(parse_equ_hex);
    TEST(parse_equ_label_subtraction);
    TEST(parse_resb);
    TEST(parse_resw);
    TEST(parse_resd);
    TEST(parse_weak_hidden_directives);
    TEST(parse_resq);
    TEST(parse_db_string);
    TEST(parse_db_string_escapes);
    TEST(parse_db_mixed);
    TEST(parse_bss_section);
    TEST(parse_rodata_section);
    
    /* Character literal tests */
    TEST(parse_char_literal);
    TEST(parse_char_literal_escape);
    TEST(parse_char_literal_in_db);
    TEST(parse_char_literal_in_instruction);
    
    /* Align directive tests */
    TEST(parse_align);
    TEST(parse_align_various);
    TEST(parse_align_data);
    
    /* incbin directive tests */
    TEST(parse_incbin);
    
    /* UTF-8 string tests */
    TEST(parse_utf8_string);
    TEST(parse_utf8_chinese);
    TEST(parse_utf8_mixed);
    
    /* String concatenation tests */
    TEST(parse_string_concat);
    TEST(parse_string_concat_multiple);
    TEST(parse_string_concat_with_escapes);
    
    /* Common symbol tests */
    TEST(parse_comm);
    TEST(parse_comm_with_alignment);
    TEST(parse_lcomm);
    TEST(parse_lcomm_with_alignment);
}

/* Main entry point */
int main(void) {
    test_init();
    RUN_TEST_SUITE(parser);
    test_report();
    return test_final_status();
}
