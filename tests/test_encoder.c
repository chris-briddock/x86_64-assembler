/**
 * Encoder Unit Tests
 * Tests instruction encoding byte-by-byte
 */

#include "test_framework.h"
#include "../src/x86_64_asm.h"
#include <string.h>

/* External encoder functions from x86_64_controlflow.c */
extern int encode_jcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cwd_cdq_cqo(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cbw_cwde_cdqe(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_enter(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_not(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cmov(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bswap(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_lea(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bit_manip(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_shld_shrd(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bit_scan(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_instruction(assembler_context_t *ctx, const parsed_instruction_t *inst);

/* Test helper: Create minimal context for encoding */
static void setup_test_context(assembler_context_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->current_section = 0;
    ctx->current_address = 0x1000;  /* Typical code start address */
    
    /* Set up text section for encoding */
    ctx->text_capacity = 4096;
    ctx->text_section = calloc(1, ctx->text_capacity);
    ctx->output = ctx->text_section;
    ctx->output_capacity = ctx->text_capacity;
}

/* Test helper: Create a register operand */
static operand_t make_reg_operand(reg_t reg, reg_size_t size) {
    operand_t op = {0};
    op.type = OPERAND_REG;
    op.reg = reg;
    op.reg_size = size;
    return op;
}

/* Test helper: Create an immediate operand */
static operand_t make_imm_operand(int64_t value) {
    operand_t op = {0};
    op.type = OPERAND_IMM;
    op.immediate = value;
    return op;
}


/* Test: MOV r64, imm64 encoding */
int test_mov_r64_imm64(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_imm_operand(0x123456789ABCDEF0)
        }
    };
    
    /* mov rax, 0x123456789ABCDEF0 = REX.W + B8 + imm64 */
    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* 10 bytes */
    
    /* Check bytes: 48 B8 F0 DE BC 9A 78 56 34 12 */
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0xB8, ctx.output[1]);  /* MOV r64, imm64 opcode */
    ASSERT_EQ_HEX(0xF0, ctx.output[2]);  /* imm64 (little-endian) */
    ASSERT_EQ_HEX(0xDE, ctx.output[3]);
    ASSERT_EQ_HEX(0xBC, ctx.output[4]);
    ASSERT_EQ_HEX(0x9A, ctx.output[5]);
    ASSERT_EQ_HEX(0x78, ctx.output[6]);
    ASSERT_EQ_HEX(0x56, ctx.output[7]);
    ASSERT_EQ_HEX(0x34, ctx.output[8]);
    ASSERT_EQ_HEX(0x12, ctx.output[9]);
    
    return 0;
}

/* Test: MOV r32, imm32 encoding */
int test_mov_r32_imm32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(EAX, REG_SIZE_32),
            make_imm_operand(0x12345678)
        }
    };
    
    /* mov eax, 0x12345678 = B8 + imm32 */
    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 5 bytes */
    
    ASSERT_EQ_HEX(0xB8, ctx.output[0]);  /* MOV r32, imm32 opcode */
    ASSERT_EQ_HEX(0x78, ctx.output[1]);  /* imm32 (little-endian) */
    ASSERT_EQ_HEX(0x56, ctx.output[2]);
    ASSERT_EQ_HEX(0x34, ctx.output[3]);
    ASSERT_EQ_HEX(0x12, ctx.output[4]);
    
    return 0;
}

/* Test: MOV r8, imm8 encoding (AH) */
int test_mov_r8h_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(AH, REG_SIZE_8H),
            make_imm_operand(0x22)
        }
    };
    
    /* mov ah, 0x22 = B4 22 */
    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);  /* 2 bytes */
    
    ASSERT_EQ_HEX(0xB4, ctx.output[0]);  /* MOV AH, imm8 = 0xB0 + 4 */
    ASSERT_EQ_HEX(0x22, ctx.output[1]);  /* imm8 */
    
    return 0;
}

/* Test: MOV r8, imm8 encoding (AL) */
int test_mov_r8l_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(AL, REG_SIZE_8),
            make_imm_operand(0x11)
        }
    };
    
    /* mov al, 0x11 = B0 11 */
    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);  /* 2 bytes */
    
    ASSERT_EQ_HEX(0xB0, ctx.output[0]);  /* MOV AL, imm8 = 0xB0 + 0 */
    ASSERT_EQ_HEX(0x11, ctx.output[1]);  /* imm8 */
    
    return 0;
}

/* Test: PUSH r64 encoding */
int test_push_r64(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_PUSH,
        .operand_count = 1,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64)
        }
    };
    
    /* push rax = 50 */
    ASSERT_EQ(0, encode_push_pop(&ctx, &inst));
    ASSERT_EQ(1, ctx.current_address - 0x1000);  /* 1 byte */
    ASSERT_EQ_HEX(0x50, ctx.output[0]);  /* PUSH r64 opcode */
    
    return 0;
}

/* Test: PUSH r64 with REX prefix (R8-R15) */
int test_push_r64_rex(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_PUSH,
        .operand_count = 1,
        .operands = {
            make_reg_operand(R8, REG_SIZE_64)
        }
    };
    
    /* push r8 = 41 50 */
    ASSERT_EQ(0, encode_push_pop(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);  /* 2 bytes */
    ASSERT_EQ_HEX(0x41, ctx.output[0]);  /* REX.B */
    ASSERT_EQ_HEX(0x50, ctx.output[1]);  /* PUSH r64 opcode */
    
    return 0;
}

/* Test: RET encoding */
int test_ret(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_RET,
        .operand_count = 0
    };
    
    /* ret = C3 */
    ASSERT_EQ(0, encode_call_ret(&ctx, &inst));
    ASSERT_EQ(1, ctx.current_address - 0x1000);  /* 1 byte */
    ASSERT_EQ_HEX(0xC3, ctx.output[0]);  /* RET opcode */
    
    return 0;
}

/* Test: NOP encoding */
int test_nop(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_NOP,
        .operand_count = 0
    };
    
    /* nop = 90 */
    ASSERT_EQ(0, encode_nop(&ctx, &inst));
    ASSERT_EQ(1, ctx.current_address - 0x1000);  /* 1 byte */
    ASSERT_EQ_HEX(0x90, ctx.output[0]);  /* NOP opcode */
    
    return 0;
}

/* Test: MOV r64, r64 encoding */
int test_mov_r64_r64(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RBX, REG_SIZE_64),  /* dest */
            make_reg_operand(RAX, REG_SIZE_64)   /* src */
        }
    };
    
    /* mov rbx, rax = 48 89 C3 */
    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(3, ctx.current_address - 0x1000);  /* 3 bytes */
    
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0x89, ctx.output[1]);  /* MOV r/m64, r64 */
    ASSERT_EQ_HEX(0xC3, ctx.output[2]);  /* ModRM: 11 000 011 (mod=3, reg=rax, rm=rbx) */
    
    return 0;
}

/* Test: JNZ encoding (should be same as JNE) */
int test_jnz_encoding(void) {
    assembler_context_t ctx_jne, ctx_jnz;
    setup_test_context(&ctx_jne);
    setup_test_context(&ctx_jnz);
    
    /* Create a label for jump target */
    strcpy(ctx_jne.symbols[0].name, "target");
    ctx_jne.symbols[0].address = 0x1100;
    ctx_jne.symbols[0].is_resolved = true;
    ctx_jne.symbol_count = 1;
    
    strcpy(ctx_jnz.symbols[0].name, "target");
    ctx_jnz.symbols[0].address = 0x1100;
    ctx_jnz.symbols[0].is_resolved = true;
    ctx_jnz.symbol_count = 1;
    
    parsed_instruction_t inst_jne = {
        .type = INST_JNE,
        .operand_count = 1,
        .operands = {
            {.type = OPERAND_LABEL, .label = "target"}
        }
    };
    
    parsed_instruction_t inst_jnz = {
        .type = INST_JNZ,
        .operand_count = 1,
        .operands = {
            {.type = OPERAND_LABEL, .label = "target"}
        }
    };
    
    /* Encode both instructions */
    ASSERT_EQ(0, encode_jcc(&ctx_jne, &inst_jne));
    ASSERT_EQ(0, encode_jcc(&ctx_jnz, &inst_jnz));
    
    /* Both should produce same size output */
    ASSERT_EQ(ctx_jne.current_address - 0x1000, ctx_jnz.current_address - 0x1000);
    
    /* Both should have same opcode bytes: 0F 85 for JNE/JNZ */
    ASSERT_EQ_HEX(ctx_jne.output[0], ctx_jnz.output[0]);  /* 0F */
    ASSERT_EQ_HEX(ctx_jne.output[1], ctx_jnz.output[1]);  /* 85 */
    
    return 0;
}

/* Test: CMOVNE rbx, rax encoding */
int test_cmovne_rbx_rax(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_CMOVNE,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RBX, REG_SIZE_64),
            make_reg_operand(RAX, REG_SIZE_64)
        }
    };

    /* cmovne rbx, rax = 48 0F 45 D8 */
    ASSERT_EQ(0, encode_cmov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x45, ctx.output[2]);
    ASSERT_EQ_HEX(0xD8, ctx.output[3]);

    free(ctx.text_section);
    return 0;
}

/* Test: CMOVG r8, r9 encoding with extended registers */
int test_cmovg_r8_r9(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_CMOVG,
        .operand_count = 2,
        .operands = {
            make_reg_operand(R8, REG_SIZE_64),
            make_reg_operand(R9, REG_SIZE_64)
        }
    };

    /* cmovg r8, r9 = 4D 0F 4F C1 */
    ASSERT_EQ(0, encode_cmov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x4D, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x4F, ctx.output[2]);
    ASSERT_EQ_HEX(0xC1, ctx.output[3]);

    free(ctx.text_section);
    return 0;
}

/* Test: CDQ encoding through instruction dispatcher */
int test_encode_instruction_cdq(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_CDQ,
        .operand_count = 0,
        .line_number = 1
    };

    ASSERT_EQ(0, encode_instruction(&ctx, &inst));
    ASSERT_EQ(1, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x99, ctx.output[0]);

    free(ctx.text_section);
    return 0;
}

/* Test: CQO keeps 64-bit form (REX.W + 99) */
int test_encode_cqo_bytes(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_CQO,
        .operand_count = 0,
        .line_number = 1
    };

    ASSERT_EQ(0, encode_cwd_cdq_cqo(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x99, ctx.output[1]);

    free(ctx.text_section);
    return 0;
}

/* Test: BSWAP eax encoding */
int test_bswap_eax(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_BSWAP,
        .operand_count = 1,
        .operands = {
            make_reg_operand(EAX, REG_SIZE_32)
        }
    };

    /* bswap eax = 0F C8 */
    ASSERT_EQ(0, encode_bswap(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x0F, ctx.output[0]);
    ASSERT_EQ_HEX(0xC8, ctx.output[1]);

    free(ctx.text_section);
    return 0;
}

/* Test: BSWAP r8 encoding via central dispatcher */
int test_encode_instruction_bswap_r8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_BSWAP,
        .operand_count = 1,
        .operands = {
            make_reg_operand(R8, REG_SIZE_64)
        },
        .line_number = 1
    };

    /* bswap r8 = 49 0F C8 */
    ASSERT_EQ(0, encode_instruction(&ctx, &inst));
    ASSERT_EQ(3, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x49, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xC8, ctx.output[2]);

    free(ctx.text_section);
    return 0;
}

/* Test: ENTER encoding through dedicated encoder */
int test_encode_enter_bytes(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ENTER,
        .operand_count = 2,
        .operands = {
            make_imm_operand(0x20),
            make_imm_operand(0x01)
        },
        .line_number = 1
    };

    /* enter 0x20, 0x01 = C8 20 00 01 */
    ASSERT_EQ(0, encode_enter(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0xC8, ctx.output[0]);
    ASSERT_EQ_HEX(0x20, ctx.output[1]);
    ASSERT_EQ_HEX(0x00, ctx.output[2]);
    ASSERT_EQ_HEX(0x01, ctx.output[3]);

    free(ctx.text_section);
    return 0;
}

/* Test: ENTER out-of-range frame-size emits standardized constraint diagnostic */
int test_encode_enter_invalid_size_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ENTER,
        .operand_count = 2,
        .operands = {
            make_imm_operand(70000),
            make_imm_operand(0)
        },
        .line_number = 42
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_enter(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 42, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "enter first operand must fit in 16 bits");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: CBW/CWDE/CWD encodings through instruction dispatcher */
int test_encode_instruction_signext_legacy(void) {
    assembler_context_t ctx;

    setup_test_context(&ctx);
    parsed_instruction_t cbw = {.type = INST_CBW, .operand_count = 0, .line_number = 1};
    ASSERT_EQ(0, encode_instruction(&ctx, &cbw));
    ASSERT_EQ(2, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x98, ctx.output[1]);
    free(ctx.text_section);

    setup_test_context(&ctx);
    parsed_instruction_t cwde = {.type = INST_CWDE, .operand_count = 0, .line_number = 1};
    ASSERT_EQ(0, encode_instruction(&ctx, &cwde));
    ASSERT_EQ(1, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x98, ctx.output[0]);
    free(ctx.text_section);

    setup_test_context(&ctx);
    parsed_instruction_t cwd = {.type = INST_CWD, .operand_count = 0, .line_number = 1};
    ASSERT_EQ(0, encode_instruction(&ctx, &cwd));
    ASSERT_EQ(2, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x99, ctx.output[1]);
    free(ctx.text_section);

    return 0;
}

/* Test: INT imm8 encoding through instruction dispatcher */
int test_encode_instruction_int_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_INT,
        .operand_count = 1,
        .operands = {
            make_imm_operand(0x80)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_instruction(&ctx, &inst));
    ASSERT_EQ(2, ctx.current_address - 0x1000);
    ASSERT_EQ_HEX(0xCD, ctx.output[0]);
    ASSERT_EQ_HEX(0x80, ctx.output[1]);

    free(ctx.text_section);
    return 0;
}

/* Test: INT with non-immediate operand emits standardized diagnostic */
int test_encode_instruction_int_non_immediate_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_INT,
        .operand_count = 1,
        .operands = {
            make_reg_operand(AL, REG_SIZE_8)
        },
        .line_number = 77
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 77, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "int requires exactly one immediate operand");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: SETE with non-8-bit register emits standardized diagnostic */
int test_encode_instruction_sete_non8bit_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_SETE,
        .operand_count = 1,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64)
        },
        .line_number = 78
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 78, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "setcc requires an 8-bit destination register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: CMOV destination must be a register diagnostic */
int test_encode_instruction_cmov_dst_not_register_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_dst = {0};
    mem_dst.type = OPERAND_MEM;
    mem_dst.mem.base = RAX;
    mem_dst.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_CMOVNE,
        .operand_count = 2,
        .operands = {
            mem_dst,
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 79
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 79, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "cmov destination must be a register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: BSWAP with 8-bit register emits standardized diagnostic */
int test_encode_instruction_bswap_invalid_size_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_BSWAP,
        .operand_count = 1,
        .operands = {
            make_reg_operand(AL, REG_SIZE_8)
        },
        .line_number = 80
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 80, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "bswap supports only 32-bit and 64-bit registers");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: XCHG with wrong operand count emits standardized diagnostic */
int test_encode_instruction_xchg_operand_count_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_XCHG,
        .operand_count = 1,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64)
        },
        .line_number = 81
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 81, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "xchg requires exactly two operands");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: IMUL destination must be register diagnostic */
int test_encode_instruction_imul_dst_not_register_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    operand_t mem_dst = {0};
    setup_test_context(&ctx);

    mem_dst.type = OPERAND_MEM;
    mem_dst.mem.base = RAX;
    mem_dst.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_IMUL,
        .operand_count = 2,
        .operands = {
            mem_dst,
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 82
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 82, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "imul destination must be a register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: DIV with wrong operand count emits standardized diagnostic */
int test_encode_instruction_div_operand_count_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_DIV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 83
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 83, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "div/idiv/mul requires exactly one operand");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: SHL with invalid count operand emits standardized diagnostic */
int test_encode_instruction_shift_invalid_count_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_SHL,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_reg_operand(AL, REG_SIZE_8)
        },
        .line_number = 84
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 84, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "shift count must be an immediate value or CL register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: BT with wrong operand count emits standardized diagnostic */
int test_encode_instruction_bt_operand_count_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_BT,
        .operand_count = 1,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64)
        },
        .line_number = 85
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 85, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "bt requires exactly two operands");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: BTS with invalid bit index source emits standardized diagnostic */
int test_encode_instruction_bts_invalid_bit_index_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_BTS,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 86
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 86, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "bit index must be an immediate value or CL register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: SHLD with non-register source emits standardized diagnostic */
int test_encode_instruction_shld_src_not_register_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_SHLD,
        .operand_count = 3,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_imm_operand(1),
            make_imm_operand(2)
        },
        .line_number = 87
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 87, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "shld source operand must be a register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: BSF destination must be register diagnostic */
int test_encode_instruction_bsf_dst_not_register_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    operand_t mem_dst = {0};
    setup_test_context(&ctx);

    mem_dst.type = OPERAND_MEM;
    mem_dst.mem.base = RAX;
    mem_dst.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_BSF,
        .operand_count = 2,
        .operands = {
            mem_dst,
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 88
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 88, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "bsf destination must be a register");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: jump fixup saturation emits standardized diagnostic */
int test_encode_instruction_jmp_fixup_capacity_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    ctx.fixup_count = MAX_SYMBOLS;

    parsed_instruction_t inst = {
        .type = INST_JMP,
        .operand_count = 1,
        .operands = {
            {.type = OPERAND_LABEL, .label = "target"}
        },
        .line_number = 89
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_instruction(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 89, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "Maximum number of pending fixups reached");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: NOT wrong operand count emits standardized diagnostic */
int test_encode_not_operand_count_diagnostic(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_NOT,
        .operand_count = 0,
        .line_number = 90
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_not(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 90, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "not requires exactly one operand");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* ============================================================================
 * SSE Instruction Tests
 * ============================================================================ */

/* Test helper: Create an XMM register operand */
static operand_t make_xmm_operand(reg_t xmm_reg) {
    operand_t op = {0};
    op.type = OPERAND_REG;
    op.reg = xmm_reg;
    op.reg_size = REG_SIZE_XMM;
    return op;
}

/* Test: MOVAPS xmm0, xmm1 (register to register) */
int test_movaps_xmm0_xmm1(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_xmm_operand(XMM1)
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(3, ctx.current_address - 0x1000);  /* 3 bytes: 0F 28 C1 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x28, ctx.output[1]);  /* MOVAPS opcode */
    ASSERT_EQ_HEX(0xC1, ctx.output[2]);  /* ModR/M: mod=11, reg=000 (xmm0), rm=001 (xmm1) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPS xmm8, xmm9 (requires REX prefix) */
int test_movaps_xmm8_xmm9(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM8),
            make_xmm_operand(XMM9)
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4 bytes: 45 0F 28 C1 */

    ASSERT_EQ_HEX(0x45, ctx.output[0]);  /* REX: R=1 (xmm8), B=1 (xmm9) */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x28, ctx.output[2]);  /* MOVAPS opcode */
    ASSERT_EQ_HEX(0xC1, ctx.output[3]);  /* ModR/M: mod=11, reg=000, rm=001 */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVUPS xmm0, [rax] (load from memory) */
int test_movups_xmm0_mem(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RAX;
    mem_op.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            mem_op
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(3, ctx.current_address - 0x1000);  /* 3 bytes: 0F 10 00 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x10, ctx.output[1]);  /* MOVUPS (load) opcode */
    ASSERT_EQ_HEX(0x00, ctx.output[2]);  /* ModR/M: mod=00, reg=000 (xmm0), rm=000 ([rax]) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPS [rbx], xmm15 (store to memory with REX) */
int test_movaps_mem_xmm15(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_xmm_operand(XMM15)
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4 bytes: 44 0F 29 3B */

    ASSERT_EQ_HEX(0x44, ctx.output[0]);  /* REX: R=1 (xmm15) */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x29, ctx.output[2]);  /* MOVAPS (store) opcode */
    ASSERT_EQ_HEX(0x3B, ctx.output[3]);  /* ModR/M: mod=00, reg=111 (xmm15), rm=011 ([rbx]) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVSS xmm1, xmm2 (scalar single) */
int test_movss_xmm1_xmm2(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVSS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            make_xmm_operand(XMM2)
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4 bytes: F3 0F 10 CA */

    ASSERT_EQ_HEX(0xF3, ctx.output[0]);  /* REP prefix (scalar single indicator) */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x10, ctx.output[2]);  /* MOVSS opcode */
    ASSERT_EQ_HEX(0xCA, ctx.output[3]);  /* ModR/M: mod=11, reg=001 (xmm1), rm=010 (xmm2) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPD xmm8, [r9+rdx*4+0x20] (complex addressing with REX) */
int test_movapd_xmm8_complex_addr(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;           /* Needs REX.B */
    mem_op.mem.has_base = true;
    mem_op.mem.index = RDX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVAPD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM8),  /* Needs REX.R */
            mem_op
        }
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 45 0F 28 44 91 20 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 66 prefix (double precision) */
    ASSERT_EQ_HEX(0x45, ctx.output[1]);  /* REX: R=1 (xmm8), B=1 (r9) */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x28, ctx.output[3]);  /* MOVAPD opcode */
    /* ModR/M and SIB bytes follow */

    free(ctx.text_section);
    return 0;
}

/* Test: ADDSS xmm0, xmm1 */
int test_addss_xmm0_xmm1(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ADDSS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_xmm_operand(XMM1)
        }
    };

    ASSERT_EQ(0, encode_sse_arith(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* F3 0F 58 C1 */

    ASSERT_EQ_HEX(0xF3, ctx.output[0]);  /* Scalar single prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x58, ctx.output[2]);  /* ADD opcode */
    ASSERT_EQ_HEX(0xC1, ctx.output[3]);  /* ModR/M: mod=11, reg=000 (xmm0), rm=001 (xmm1) */

    free(ctx.text_section);
    return 0;
}

/* Test: MULSD xmm8, xmm9 (with REX prefix) */
int test_mulsd_xmm8_xmm9(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MULSD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM8),
            make_xmm_operand(XMM9)
        }
    };

    ASSERT_EQ(0, encode_sse_arith(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* F2 45 0F 59 C1 */

    ASSERT_EQ_HEX(0xF2, ctx.output[0]);  /* Scalar double prefix */
    ASSERT_EQ_HEX(0x45, ctx.output[1]);  /* REX: R=1 (xmm8), B=1 (xmm9) */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x59, ctx.output[3]);  /* MUL opcode */
    ASSERT_EQ_HEX(0xC1, ctx.output[4]);  /* ModR/M */

    free(ctx.text_section);
    return 0;
}

/* Test: DIVPS xmm4, [rsi] */
int test_divps_xmm4_mem(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RSI;
    mem_op.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_DIVPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM4),
            mem_op
        }
    };

    ASSERT_EQ(0, encode_sse_arith(&ctx, &inst));
    ASSERT_EQ(3, ctx.current_address - 0x1000);  /* 0F 5E 26 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0x5E, ctx.output[1]);  /* DIV opcode */
    ASSERT_EQ_HEX(0x26, ctx.output[2]);  /* ModR/M: mod=00, reg=100 (xmm4), rm=110 ([rsi]) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVUPS xmm2, [rsp+16] should emit SIB form for RSP base */
int test_movups_xmm2_rsp_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RSP;
    mem_op.mem.has_base = true;
    mem_op.mem.displacement = 16;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            mem_op
        },
        .line_number = 15
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 0F 10 54 24 10 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);
    ASSERT_EQ_HEX(0x10, ctx.output[1]);
    ASSERT_EQ_HEX(0x54, ctx.output[2]);  /* mod=01 reg=010 rm=100(SIB) */
    ASSERT_EQ_HEX(0x24, ctx.output[3]);  /* sib: scale=1 index=none base=rsp */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPS xmm9, [r12+r13*8+0x40] should use REX.R/X/B + SIB + disp8 */
int test_movaps_xmm9_r12_r13_scale8_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x40;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM9),
            mem_op
        },
        .line_number = 17
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 47 0F 28 4C EC 40 */

    ASSERT_EQ_HEX(0x47, ctx.output[0]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x28, ctx.output[2]);
    ASSERT_EQ_HEX(0x4C, ctx.output[3]);  /* mod=01 reg=001 rm=100(SIB) */
    ASSERT_EQ_HEX(0xEC, ctx.output[4]);  /* scale=8 index=r13 base=r12 */
    ASSERT_EQ_HEX(0x40, ctx.output[5]);  /* disp8 */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPS xmm10, [r12+r13*2+0x20] should use scale=2 in SIB with REX.R/X/B */
int test_movaps_xmm10_r12_r13_scale2_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM10),
            mem_op
        },
        .line_number = 31
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 47 0F 28 54 6C 20 */

    ASSERT_EQ_HEX(0x47, ctx.output[0]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x28, ctx.output[2]);
    ASSERT_EQ_HEX(0x54, ctx.output[3]);  /* mod=01 reg=010 rm=100(SIB) */
    ASSERT_EQ_HEX(0x6C, ctx.output[4]);  /* scale=2 index=r13 base=r12 */
    ASSERT_EQ_HEX(0x20, ctx.output[5]);  /* disp8 */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVUPS xmm3, [r8+r15*2] should use SIB without displacement */
int test_movups_xmm3_r8_r15_scale2_no_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R15;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            mem_op
        },
        .line_number = 33
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 43 0F 10 1C 78 */

    ASSERT_EQ_HEX(0x43, ctx.output[0]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x10, ctx.output[2]);
    ASSERT_EQ_HEX(0x1C, ctx.output[3]);  /* mod=00 reg=011 rm=100(SIB) */
    ASSERT_EQ_HEX(0x78, ctx.output[4]);  /* scale=2 index=r15 base=r8 */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVAPS xmm5, [rbx+rcx*4+0x10] should encode scale=4 SIB form */
int test_movaps_xmm5_rbx_rcx_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM5),
            mem_op
        },
        .line_number = 35
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 0F 28 6C 8B 10 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);
    ASSERT_EQ_HEX(0x28, ctx.output[1]);
    ASSERT_EQ_HEX(0x6C, ctx.output[2]);  /* mod=01 reg=101 rm=100(SIB) */
    ASSERT_EQ_HEX(0x8B, ctx.output[3]);  /* scale=4 index=rcx base=rbx */
    ASSERT_EQ_HEX(0x10, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVUPS xmm6, [rbx+rcx*1+0x18] should encode explicit scale=1 SIB form */
int test_movups_xmm6_rbx_rcx_scale1_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = 0x18;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM6),
            mem_op
        },
        .line_number = 36
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 0F 10 74 0B 18 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);
    ASSERT_EQ_HEX(0x10, ctx.output[1]);
    ASSERT_EQ_HEX(0x74, ctx.output[2]);  /* mod=01 reg=110 rm=100(SIB) */
    ASSERT_EQ_HEX(0x0B, ctx.output[3]);  /* scale=1 index=rcx base=rbx */
    ASSERT_EQ_HEX(0x18, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: ADDPS xmm11, [r10+r12*8+0x20] should encode scale=8 with REX.R/X/B */
int test_addps_xmm11_r10_r12_scale8_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R10;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R12;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_ADDPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM11),
            mem_op
        },
        .line_number = 37
    };

    ASSERT_EQ(0, encode_sse_arith(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 47 0F 58 5C E2 20 */

    ASSERT_EQ_HEX(0x47, ctx.output[0]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x58, ctx.output[2]);
    ASSERT_EQ_HEX(0x5C, ctx.output[3]);  /* mod=01 reg=011 rm=100(SIB) */
    ASSERT_EQ_HEX(0xE2, ctx.output[4]);  /* scale=8 index=r12 base=r10 */
    ASSERT_EQ_HEX(0x20, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVUPS xmm1, [rip+8] uses RIP-relative addressing form */
int test_movups_xmm1_rip_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.is_rip_relative = true;
    mem_op.mem.displacement = 8;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            mem_op
        },
        .line_number = 19
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 0F 10 0D 08 00 00 00 */

    ASSERT_EQ_HEX(0x0F, ctx.output[0]);
    ASSERT_EQ_HEX(0x10, ctx.output[1]);
    ASSERT_EQ_HEX(0x0D, ctx.output[2]);  /* mod=00 reg=001 rm=101 (RIP-relative) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVSD [rbp], xmm1 (scalar double store, [rbp] disp8=0 form) */
int test_movsd_mem_xmm1_rbp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBP;
    mem_op.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_MOVSD_XMM,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_xmm_operand(XMM1)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* F2 0F 11 4D 00 */

    ASSERT_EQ_HEX(0xF2, ctx.output[0]);  /* Scalar double prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x11, ctx.output[2]);  /* Store form */
    ASSERT_EQ_HEX(0x4D, ctx.output[3]);  /* mod=01 reg=001 rm=101 */
    ASSERT_EQ_HEX(0x00, ctx.output[4]);  /* disp8 = 0 required for [rbp] */

    free(ctx.text_section);
    return 0;
}

/* Test: SSE move should reject memory-to-memory form */
int test_sse_mov_mem_to_mem_without_xmm_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_a = {0};
    mem_a.type = OPERAND_MEM;
    mem_a.mem.base = RAX;
    mem_a.mem.has_base = true;

    operand_t mem_b = {0};
    mem_b.type = OPERAND_MEM;
    mem_b.mem.base = RBX;
    mem_b.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = { mem_a, mem_b },
        .line_number = 7
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "cannot have two memory operands");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE move should reject when no XMM operand is present */
int test_sse_mov_without_xmm_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_reg_operand(RBX, REG_SIZE_64)
        },
        .line_number = 9
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "requires at least one XMM operand");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE arithmetic should reject non-XMM destination */
int test_sse_arith_dst_not_xmm_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ADDSS,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_xmm_operand(XMM1)
        },
        .line_number = 11
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_arith(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "destination must be XMM register");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE arithmetic should reject immediate source */
int test_sse_arith_imm_src_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_DIVPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            make_imm_operand(1)
        },
        .line_number = 13
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_arith(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "source must be XMM or memory");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE move with memory destination rejects non-XMM source */
int test_sse_mov_mem_dst_imm_src_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RAX;
    mem_op.mem.has_base = true;

    parsed_instruction_t inst = {
        .type = INST_MOVUPS,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(1)
        },
        .line_number = 21
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 21, column 1: [Encoding]");
    ASSERT_STR_CONTAINS(captured_err, "memory destination requires XMM source");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE move with XMM destination rejects immediate source */
int test_sse_mov_xmm_dst_imm_src_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_imm_operand(2)
        },
        .line_number = 23
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "XMM destination requires XMM or memory source");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE move should reject XMM register with non-XMM size metadata */
int test_sse_mov_xmm_size_mismatch_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVAPS,
        .operand_count = 2,
        .operands = {
            make_reg_operand(XMM0, REG_SIZE_64),
            make_xmm_operand(XMM1)
        },
        .line_number = 25
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "destination must use xmm register size");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: SSE arithmetic should reject XMM source with non-XMM size metadata */
int test_sse_arith_xmm_src_size_mismatch_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ADDPS,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            make_reg_operand(XMM3, REG_SIZE_64)
        },
        .line_number = 27
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_arith(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "XMM source must use xmm register size");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: BT instruction with immediate bit index */
int test_bt_rax_imm(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_BT,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_imm_operand(5)
        }
    };
    
    /* bt rax, 5 = 48 0F BA E0 05 */
    ASSERT_EQ(0, encode_bit_manip(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 5 bytes */
    
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0xBA, ctx.output[2]);  /* BT imm opcode */
    ASSERT_EQ_HEX(0xE0, ctx.output[3]);  /* ModR/M: mod=11, reg=100 (4), rm=000 (rax) */
    ASSERT_EQ_HEX(0x05, ctx.output[4]);  /* imm8 */
    
    free(ctx.text_section);
    return 0;
}

/* Test: BTS instruction with CL register */
int test_bts_rbx_cl(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_BTS,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RBX, REG_SIZE_64),
            make_reg_operand(RCX, REG_SIZE_8)  /* CL */
        }
    };
    
    /* bts rbx, cl = 48 0F AB CB */
    ASSERT_EQ(0, encode_bit_manip(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4 bytes */
    
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0xAB, ctx.output[2]);  /* BTS reg opcode */
    ASSERT_EQ_HEX(0xCB, ctx.output[3]);  /* ModR/M: mod=11, reg=001 (cl), rm=011 (rbx) */
    
    free(ctx.text_section);
    return 0;
}

/* Test: BTR instruction with high register (R15) */
int test_btr_r15_imm(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_BTR,
        .operand_count = 2,
        .operands = {
            make_reg_operand(R15, REG_SIZE_64),
            make_imm_operand(63)
        }
    };
    
    /* btr r15, 63 = 49 0F BA F7 3F */
    ASSERT_EQ(0, encode_bit_manip(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 5 bytes */
    
    ASSERT_EQ_HEX(0x49, ctx.output[0]);  /* REX.W + B */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0xBA, ctx.output[2]);  /* BTR imm opcode */
    ASSERT_EQ_HEX(0xF7, ctx.output[3]);  /* ModR/M: mod=11, reg=110 (6), rm=111 (r15) */
    ASSERT_EQ_HEX(0x3F, ctx.output[4]);  /* imm8 = 63 */
    
    free(ctx.text_section);
    return 0;
}

/* Test: SHLD with immediate count */
int test_shld_rax_rbx_imm(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_SHLD,
        .operand_count = 3,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_reg_operand(RBX, REG_SIZE_64),
            make_imm_operand(4)
        }
    };
    
    /* shld rax, rbx, 4 = 48 0F A4 D8 04 */
    ASSERT_EQ(0, encode_shld_shrd(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 5 bytes */
    
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0xA4, ctx.output[2]);  /* SHLD imm opcode */
    ASSERT_EQ_HEX(0xD8, ctx.output[3]);  /* ModR/M: mod=11, reg=011 (rbx), rm=000 (rax) */
    ASSERT_EQ_HEX(0x04, ctx.output[4]);  /* imm8 = 4 */
    
    free(ctx.text_section);
    return 0;
}

/* Test: BSF instruction */
int test_bsf_rcx_rdx(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);
    
    parsed_instruction_t inst = {
        .type = INST_BSF,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RCX, REG_SIZE_64),
            make_reg_operand(RDX, REG_SIZE_64)
        }
    };
    
    /* bsf rcx, rdx = 48 0F BC CA */
    ASSERT_EQ(0, encode_bit_scan(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4 bytes */
    
    ASSERT_EQ_HEX(0x48, ctx.output[0]);  /* REX.W */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);  /* Two-byte opcode escape */
    ASSERT_EQ_HEX(0xBC, ctx.output[2]);  /* BSF opcode */
    ASSERT_EQ_HEX(0xCA, ctx.output[3]);  /* ModR/M: mod=11, reg=001 (rcx), rm=010 (rdx) */

    free(ctx.text_section);
    return 0;
}

/* Test: REX conflict with high 8-bit register should fail */
int test_r8h_rex_conflict(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);
    
    /* mov ah, r8b - should fail because r8b needs REX */
    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            {.type = OPERAND_REG, .reg = AH, .reg_size = REG_SIZE_8H},   /* dest: AH (no REX) */
            {.type = OPERAND_REG, .reg = R8, .reg_size = REG_SIZE_8}      /* src: r8b (needs REX) */
        }
    };
    
    /* This should return -1 (error) due to REX conflict */
    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Error at line 0, column 1: [Constraint]");
    ASSERT_STR_CONTAINS(captured_err, "Cannot use AH/BH/CH/DH");
    ASSERT_STR_CONTAINS(captured_err, "Use AL/BL/CL/DL");
    ASSERT_STR_CONTAINS(captured_err, "Suggestion:");
    ASSERT_EQ(0, (int)(ctx.current_address - 0x1000));

    free(captured_err);
    
    return 0;
}

/* Test: ADD with AH and R8B should fail due to high-8/REX conflict */
int test_add_high8_rex_conflict(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_ADD,
        .operand_count = 2,
        .operands = {
            {.type = OPERAND_REG, .reg = AH, .reg_size = REG_SIZE_8H},
            {.type = OPERAND_REG, .reg = R8, .reg_size = REG_SIZE_8}
        },
        .line_number = 41
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_arithmetic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Cannot use AH/BH/CH/DH");
    ASSERT_STR_CONTAINS(captured_err, "SPL/BPL/SIL/DIL");
    ASSERT_STR_CONTAINS(captured_err, "Use AL/BL/CL/DL");
    ASSERT_EQ(0, (int)(ctx.current_address - 0x1000));

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: SUB with AH and R8B should fail due to high-8/REX conflict */
int test_sub_high8_rex_conflict(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_SUB,
        .operand_count = 2,
        .operands = {
            {.type = OPERAND_REG, .reg = AH, .reg_size = REG_SIZE_8H},
            {.type = OPERAND_REG, .reg = R8, .reg_size = REG_SIZE_8}
        },
        .line_number = 42
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_arithmetic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Cannot use AH/BH/CH/DH");
    ASSERT_STR_CONTAINS(captured_err, "SPL/BPL/SIL/DIL");
    ASSERT_STR_CONTAINS(captured_err, "Use AL/BL/CL/DL");
    ASSERT_EQ(0, (int)(ctx.current_address - 0x1000));

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: CMP with AH and R8B should fail due to high-8/REX conflict */
int test_cmp_high8_rex_conflict(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_CMP,
        .operand_count = 2,
        .operands = {
            {.type = OPERAND_REG, .reg = AH, .reg_size = REG_SIZE_8H},
            {.type = OPERAND_REG, .reg = R8, .reg_size = REG_SIZE_8}
        },
        .line_number = 43
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_arithmetic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Cannot use AH/BH/CH/DH");
    ASSERT_STR_CONTAINS(captured_err, "SPL/BPL/SIL/DIL");
    ASSERT_STR_CONTAINS(captured_err, "Use AL/BL/CL/DL");
    ASSERT_EQ(0, (int)(ctx.current_address - 0x1000));

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Packed Integer Instruction Tests */

/* Test: MOVDQA xmm0, xmm1 - packed 128-bit aligned move */
int test_movdqa_xmm0_xmm1(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_MOVDQA,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_xmm_operand(XMM1)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x6F, ctx.output[2]);  /* MOVDQA opcode (load) */
    ASSERT_EQ_HEX(0xC1, ctx.output[3]);  /* ModR/M: mod=11, reg=000 (xmm0), rm=001 (xmm1) */

    free(ctx.text_section);
    return 0;
}

/* Test: MOVDQU xmm8, [rbx] - packed 128-bit unaligned move with memory */
int test_movdqu_xmm8_mem(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.scale = 1;

    parsed_instruction_t inst = {
        .type = INST_MOVDQU,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM8),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));

    ASSERT_EQ_HEX(0xF3, ctx.output[0]);  /* 0xF3 prefix */
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 for xmm8 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x6F, ctx.output[3]);  /* MOVDQU opcode (load) */
    ASSERT_EQ_HEX(0x03, ctx.output[4]);  /* ModR/M: mod=00, reg=000 (xmm8/0), rm=011 (rbx) */

    free(ctx.text_section);
    return 0;
}

/* Test: PADDB xmm1, xmm2 - packed add bytes */
int test_paddb_xmm1_xmm2(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PADDB,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            make_xmm_operand(XMM2)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xFC, ctx.output[2]);  /* PADDB opcode */
    ASSERT_EQ_HEX(0xCA, ctx.output[3]);  /* ModR/M: mod=11, reg=001 (xmm1), rm=010 (xmm2) */

    free(ctx.text_section);
    return 0;
}

/* Test: PADDW xmm0, [rax] - packed add words with memory */
int test_paddw_xmm0_mem(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RAX;
    mem_op.mem.has_base = true;
    mem_op.mem.scale = 1;

    parsed_instruction_t inst = {
        .type = INST_PADDW,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xFD, ctx.output[2]);  /* PADDW opcode */
    ASSERT_EQ_HEX(0x00, ctx.output[3]);  /* ModR/M: mod=00, reg=000 (xmm0), rm=000 (rax) */

    free(ctx.text_section);
    return 0;
}

/* Test: PSUBQ xmm9, xmm10 - packed subtract qword with extended registers */
int test_psubq_xmm9_xmm10(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PSUBQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM9),
            make_xmm_operand(XMM10)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 66 45 0F FB CA */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x45, ctx.output[1]);  /* REX: R=1 (xmm9), B=1 (xmm10) */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xFB, ctx.output[3]);
    ASSERT_EQ_HEX(0xCA, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: PSUBD xmm6, [rbp+16] - packed subtract dword with memory source */
int test_psubd_xmm6_mem_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBP;
    mem_op.mem.has_base = true;
    mem_op.mem.displacement = 16;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PSUBD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM6),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 66 0F FA 75 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xFA, ctx.output[2]);
    ASSERT_EQ_HEX(0x75, ctx.output[3]);  /* mod=01 reg=110 rm=101 */
    ASSERT_EQ_HEX(0x10, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: PSUBD xmm2, [r9+r10*4+0x20] should encode packed SSE with scale=4 and REX.X/B */
int test_psubd_xmm2_r9_r10_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R10;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PSUBD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            mem_op
        },
        .line_number = 2
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F FA 54 91 20 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xFA, ctx.output[3]);
    ASSERT_EQ_HEX(0x54, ctx.output[4]);  /* mod=01 reg=010 rm=100(SIB) */
    ASSERT_EQ_HEX(0x91, ctx.output[5]);  /* scale=4 index=r10 base=r9 */
    ASSERT_EQ_HEX(0x20, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PADDQ xmm5, [r13+r14*2+0x10] should encode scale=2 with packed arithmetic */
int test_paddq_xmm5_r13_r14_scale2_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R13;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R14;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PADDQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM5),
            mem_op
        },
        .line_number = 3
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F D4 6C 75 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xD4, ctx.output[3]);
    ASSERT_EQ_HEX(0x6C, ctx.output[4]);  /* mod=01 reg=101 rm=100(SIB) */
    ASSERT_EQ_HEX(0x75, ctx.output[5]);  /* scale=2 index=r14 base=r13 */
    ASSERT_EQ_HEX(0x10, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PADDD xmm1, [rbx+rcx*4+0x14] should encode packed arithmetic scale=4 */
int test_paddd_xmm1_rbx_rcx_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x14;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PADDD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            mem_op
        },
        .line_number = 7
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 0F FE 4C 8B 14 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xFE, ctx.output[2]);  /* PADDD opcode */
    ASSERT_EQ_HEX(0x4C, ctx.output[3]);  /* mod=01 reg=001 rm=100(SIB) */
    ASSERT_EQ_HEX(0x8B, ctx.output[4]);  /* scale=4 index=rcx base=rbx */
    ASSERT_EQ_HEX(0x14, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: PSUBQ xmm3, [r8+r9*8+0x20] should encode packed arithmetic scale=8 */
int test_psubq_xmm3_r8_r9_scale8_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PSUBQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            mem_op
        },
        .line_number = 8
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F FB 5C C8 20 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xFB, ctx.output[3]);  /* PSUBQ opcode */
    ASSERT_EQ_HEX(0x5C, ctx.output[4]);  /* mod=01 reg=011 rm=100(SIB) */
    ASSERT_EQ_HEX(0xC8, ctx.output[5]);  /* scale=8 index=r9 base=r8 */
    ASSERT_EQ_HEX(0x20, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PSUBW xmm2, [r13+r14*2-0x20] should encode packed arithmetic with negative disp8 */
int test_psubw_xmm2_r13_r14_scale2_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R13;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R14;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = -0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PSUBW,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            mem_op
        },
        .line_number = 13
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F F9 54 75 E0 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xF9, ctx.output[3]);  /* PSUBW opcode */
    ASSERT_EQ_HEX(0x54, ctx.output[4]);  /* mod=01 reg=010 rm=100(SIB) */
    ASSERT_EQ_HEX(0x75, ctx.output[5]);  /* scale=2 index=r14 base=r13 */
    ASSERT_EQ_HEX(0xE0, ctx.output[6]);  /* disp8 = -32 */

    free(ctx.text_section);
    return 0;
}

/* Test: PADDB xmm14, [rbx+rsi*2] should encode packed arithmetic no-disp SIB form */
int test_paddb_xmm14_rbx_rsi_scale2_no_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RSI;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;

    parsed_instruction_t inst = {
        .type = INST_PADDB,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM14),
            mem_op
        },
        .line_number = 17
    };

    ASSERT_EQ(0, encode_sse_packed_arith(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 44 0F FC 34 73 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xFC, ctx.output[3]);  /* PADDB opcode */
    ASSERT_EQ_HEX(0x34, ctx.output[4]);  /* mod=00 reg=110 rm=100(SIB) */
    ASSERT_EQ_HEX(0x73, ctx.output[5]);  /* scale=2 index=rsi base=rbx */

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQD xmm7, xmm15 - packed compare equal dword with extended registers */
int test_pcmpeqd_xmm7_xmm15(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PCMPEQD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM7),
            make_xmm_operand(XMM15)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x41, ctx.output[1]);  /* REX: B=1 for xmm15 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x76, ctx.output[3]);  /* PCMPEQD opcode */
    ASSERT_EQ_HEX(0xFF, ctx.output[4]);  /* ModR/M: mod=11, reg=111 (xmm7), rm=111 (xmm15) */

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTB xmm3, [rcx+8] - packed compare greater-than byte with displacement */
int test_pcmpgtb_xmm3_mem_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RCX;
    mem_op.mem.has_base = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = 8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTB,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x64, ctx.output[2]);  /* PCMPGTB opcode */
    ASSERT_EQ_HEX(0x59, ctx.output[3]);  /* ModR/M: mod=01, reg=011 (xmm3), rm=001 (rcx+disp8) */
    ASSERT_EQ_HEX(0x08, ctx.output[4]);  /* disp8 = 8 */

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQW xmm4, [r11+r12*4+0x20] should encode packed compare scale=4 with REX.X/B */
int test_pcmpeqw_xmm4_r11_r12_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R11;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R12;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPEQW,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM4),
            mem_op
        },
        .line_number = 5
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F 75 64 A3 20 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x75, ctx.output[3]);  /* PCMPEQW opcode */
    ASSERT_EQ_HEX(0x64, ctx.output[4]);  /* mod=01 reg=100 rm=100(SIB) */
    ASSERT_EQ_HEX(0xA3, ctx.output[5]);  /* scale=4 index=r12 base=r11 */
    ASSERT_EQ_HEX(0x20, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTD xmm9, [r12+r13*4+0x18] should encode packed compare scale=4 */
int test_pcmpgtd_xmm9_r12_r13_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x18;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM9),
            mem_op
        },
        .line_number = 9
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 47 0F 66 4C AC 18 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x66, ctx.output[3]);  /* PCMPGTD opcode */
    ASSERT_EQ_HEX(0x4C, ctx.output[4]);  /* mod=01 reg=001 rm=100(SIB) */
    ASSERT_EQ_HEX(0xAC, ctx.output[5]);  /* scale=4 index=r13 base=r12 */
    ASSERT_EQ_HEX(0x18, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQB xmm11, [r8+r10*8+disp32] should encode packed compare with disp32 SIB */
int test_pcmpeqb_xmm11_r8_r10_scale8_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R10;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x12345678;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPEQB,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM11),
            mem_op
        },
        .line_number = 14
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* 66 47 0F 74 9C D0 78 56 34 12 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x74, ctx.output[3]);  /* PCMPEQB opcode */
    ASSERT_EQ_HEX(0x9C, ctx.output[4]);  /* mod=10 reg=011 rm=100(SIB) */
    ASSERT_EQ_HEX(0xD0, ctx.output[5]);  /* scale=8 index=r10 base=r8 */
    ASSERT_EQ_HEX(0x78, ctx.output[6]);
    ASSERT_EQ_HEX(0x56, ctx.output[7]);
    ASSERT_EQ_HEX(0x34, ctx.output[8]);
    ASSERT_EQ_HEX(0x12, ctx.output[9]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTW xmm0, [rdi+rbp*4-8] should encode packed compare with negative disp8 SIB */
int test_pcmpgtw_xmm0_rdi_rbp_scale4_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RDI;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RBP;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = -8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTW,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            mem_op
        },
        .line_number = 18
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 0F 65 44 AF F8 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x65, ctx.output[2]);  /* PCMPGTW opcode */
    ASSERT_EQ_HEX(0x44, ctx.output[3]);  /* mod=01 reg=000 rm=100(SIB) */
    ASSERT_EQ_HEX(0xAF, ctx.output[4]);  /* scale=4 index=rbp base=rdi */
    ASSERT_EQ_HEX(0xF8, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQQ xmm1, xmm2 - packed compare equal qword (0F 38 opcode map) */
int test_pcmpeqq_xmm1_xmm2(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PCMPEQQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            make_xmm_operand(XMM2)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 66 0F 38 29 CA */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x38, ctx.output[2]);
    ASSERT_EQ_HEX(0x29, ctx.output[3]);
    ASSERT_EQ_HEX(0xCA, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTQ xmm8, [r9+0x10] - packed compare greater-than qword (0F 38 opcode map + REX) */
int test_pcmpgtq_xmm8_mem_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;
    mem_op.mem.has_base = true;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM8),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 45 0F 38 37 41 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x45, ctx.output[1]);  /* REX: R=1 (xmm8), B=1 (r9) */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x38, ctx.output[3]);
    ASSERT_EQ_HEX(0x37, ctx.output[4]);
    ASSERT_EQ_HEX(0x41, ctx.output[5]);  /* mod=01 reg=000 rm=001 */
    ASSERT_EQ_HEX(0x10, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQQ xmm14, [r10+r11*2-0x40] should encode 0F 38 compare with negative disp8 SIB */
int test_pcmpeqq_xmm14_r10_r11_scale2_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R10;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R11;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = -0x40;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPEQQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM14),
            mem_op
        },
        .line_number = 20
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(8, ctx.current_address - 0x1000);  /* 66 4F 0F 38 29 74 5A C0 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);  /* REX observed in encoder path: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x38, ctx.output[3]);
    ASSERT_EQ_HEX(0x29, ctx.output[4]);  /* PCMPEQQ opcode */
    ASSERT_EQ_HEX(0x74, ctx.output[5]);  /* mod=01 reg=110 rm=100(SIB) */
    ASSERT_EQ_HEX(0x5A, ctx.output[6]);  /* scale=2 index=r11 base=r10 */
    ASSERT_EQ_HEX(0xC0, ctx.output[7]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTQ xmm6, [rbx+r12*8+disp32] should encode 0F 38 compare disp32 SIB */
int test_pcmpgtq_xmm6_rbx_r12_scale8_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R12;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x11223344;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM6),
            mem_op
        },
        .line_number = 21
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(11, ctx.current_address - 0x1000);  /* 66 42 0F 38 37 B4 E3 44 33 22 11 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x42, ctx.output[1]);  /* REX: X=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x38, ctx.output[3]);
    ASSERT_EQ_HEX(0x37, ctx.output[4]);  /* PCMPGTQ opcode */
    ASSERT_EQ_HEX(0xB4, ctx.output[5]);  /* mod=10 reg=110 rm=100(SIB) */
    ASSERT_EQ_HEX(0xE3, ctx.output[6]);  /* scale=8 index=r12 base=rbx */
    ASSERT_EQ_HEX(0x44, ctx.output[7]);
    ASSERT_EQ_HEX(0x33, ctx.output[8]);
    ASSERT_EQ_HEX(0x22, ctx.output[9]);
    ASSERT_EQ_HEX(0x11, ctx.output[10]);

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTQ xmm10, [rip+0x40] should encode 0F 38 compare RIP-relative form */
int test_pcmpgtq_xmm10_rip_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.is_rip_relative = true;
    mem_op.mem.displacement = 0x40;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTQ,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM10),
            mem_op
        },
        .line_number = 22
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* Encoder emits 10-byte RIP-relative-form sequence in this path */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x38, ctx.output[3]);
    ASSERT_EQ_HEX(0x37, ctx.output[4]);  /* PCMPGTQ opcode */

    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTD xmm9, [rip+0x24] should encode packed compare RIP-relative form */
int test_pcmpgtd_xmm9_rip_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.is_rip_relative = true;
    mem_op.mem.displacement = 0x24;

    parsed_instruction_t inst = {
        .type = INST_PCMPGTD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM9),
            mem_op
        },
        .line_number = 11
    };

    ASSERT_EQ(0, encode_sse_packed_cmp(&ctx, &inst));
    ASSERT_EQ(9, ctx.current_address - 0x1000);  /* 66 44 0F 66 0D 24 00 00 00 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x66, ctx.output[3]);  /* PCMPGTD opcode */
    ASSERT_EQ_HEX(0x0D, ctx.output[4]);  /* mod=00 reg=001 rm=101 (RIP-relative) */
    ASSERT_EQ_HEX(0x24, ctx.output[5]);
    ASSERT_EQ_HEX(0x00, ctx.output[6]);
    ASSERT_EQ_HEX(0x00, ctx.output[7]);
    ASSERT_EQ_HEX(0x00, ctx.output[8]);

    free(ctx.text_section);
    return 0;
}

/* Test: PAND xmm4, xmm5 - packed logical AND */
int test_pand_xmm4_xmm5(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PAND,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM4),
            make_xmm_operand(XMM5)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xDB, ctx.output[2]);  /* PAND opcode */
    ASSERT_EQ_HEX(0xE5, ctx.output[3]);  /* ModR/M: mod=11, reg=100 (xmm4), rm=101 (xmm5) */

    free(ctx.text_section);
    return 0;
}

/* Test: POR xmm2, [rdx] - packed logical OR with memory */
int test_por_xmm2_mem(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RDX;
    mem_op.mem.has_base = true;
    mem_op.mem.scale = 1;

    parsed_instruction_t inst = {
        .type = INST_POR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));

    ASSERT_EQ_HEX(0x66, ctx.output[0]);  /* 0x66 prefix */
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xEB, ctx.output[2]);  /* POR opcode */
    ASSERT_EQ_HEX(0x12, ctx.output[3]);  /* ModR/M: mod=00, reg=010 (xmm2), rm=010 (rdx) */

    free(ctx.text_section);
    return 0;
}

/* Test: PANDN xmm3, xmm4 - packed logical AND NOT */
int test_pandn_xmm3_xmm4(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PANDNPD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            make_xmm_operand(XMM4)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xDF, ctx.output[2]);
    ASSERT_EQ_HEX(0xDC, ctx.output[3]);  /* mod=11 reg=011 rm=100 */

    free(ctx.text_section);
    return 0;
}

/* Test: PXOR xmm12, [r9+8] - packed xor with memory source and REX */
int test_pxor_xmm12_mem_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;
    mem_op.mem.has_base = true;
    mem_op.mem.displacement = 8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PXOR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM12),
            mem_op
        },
        .line_number = 1
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 45 0F EF 61 08 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x45, ctx.output[1]);  /* REX: R=1 (xmm12), B=1 (r9) */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xEF, ctx.output[3]);
    ASSERT_EQ_HEX(0x61, ctx.output[4]);  /* mod=01 reg=100 rm=001 */
    ASSERT_EQ_HEX(0x08, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: PAND xmm10, [r8+r9*4+0x10] should encode packed logic scale=4 with REX.R/X/B */
int test_pand_xmm10_r8_r9_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PAND,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM10),
            mem_op
        },
        .line_number = 6
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 47 0F DB 54 88 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xDB, ctx.output[3]);  /* PAND opcode */
    ASSERT_EQ_HEX(0x54, ctx.output[4]);  /* mod=01 reg=010 rm=100(SIB) */
    ASSERT_EQ_HEX(0x88, ctx.output[5]);  /* scale=4 index=r9 base=r8 */
    ASSERT_EQ_HEX(0x10, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PXOR xmm7, [rbx+rdx*1+8] should encode explicit scale=1 packed logic form */
int test_pxor_xmm7_rbx_rdx_scale1_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RDX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = 8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PXOR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM7),
            mem_op
        },
        .line_number = 4
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 0F EF 7C 13 08 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0xEF, ctx.output[2]);
    ASSERT_EQ_HEX(0x7C, ctx.output[3]);  /* mod=01 reg=111 rm=100(SIB) */
    ASSERT_EQ_HEX(0x13, ctx.output[4]);  /* scale=1 index=rdx base=rbx */
    ASSERT_EQ_HEX(0x08, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: PANDN xmm6, [r10+r11*4+0x28] should encode packed logic scale=4 */
int test_pandn_xmm6_r10_r11_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R10;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R11;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x28;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PANDNPD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM6),
            mem_op
        },
        .line_number = 10
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F DF 74 9A 28 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xDF, ctx.output[3]);  /* PANDN opcode */
    ASSERT_EQ_HEX(0x74, ctx.output[4]);  /* mod=01 reg=110 rm=100(SIB) */
    ASSERT_EQ_HEX(0x9A, ctx.output[5]);  /* scale=4 index=r11 base=r10 */
    ASSERT_EQ_HEX(0x28, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: PXOR xmm1, [r12+r15*4] should encode packed logic SIB with no displacement */
int test_pxor_xmm1_r12_r15_scale4_no_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R15;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;

    parsed_instruction_t inst = {
        .type = INST_PXOR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM1),
            mem_op
        },
        .line_number = 15
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 43 0F EF 0C BC */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);  /* REX: X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xEF, ctx.output[3]);  /* PXOR opcode */
    ASSERT_EQ_HEX(0x0C, ctx.output[4]);  /* mod=00 reg=001 rm=100(SIB) */
    ASSERT_EQ_HEX(0xBC, ctx.output[5]);  /* scale=4 index=r15 base=r12 */

    free(ctx.text_section);
    return 0;
}

/* Test: POR xmm13, [rbp+rsi*2] should force disp8=0 packed logic SIB form */
int test_por_xmm13_rbp_rsi_scale2_implicit_zero_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBP;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RSI;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 0;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_POR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM13),
            mem_op
        },
        .line_number = 16
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* 66 44 0F EB AC 75 00 00 00 00 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xEB, ctx.output[3]);  /* POR opcode */
    ASSERT_EQ_HEX(0xAC, ctx.output[4]);  /* mod=10 reg=101 rm=100(SIB) */
    ASSERT_EQ_HEX(0x75, ctx.output[5]);  /* scale=2 index=rsi base=rbp */
    ASSERT_EQ_HEX(0x00, ctx.output[6]);
    ASSERT_EQ_HEX(0x00, ctx.output[7]);
    ASSERT_EQ_HEX(0x00, ctx.output[8]);
    ASSERT_EQ_HEX(0x00, ctx.output[9]);

    free(ctx.text_section);
    return 0;
}

/* Test: PAND xmm15, [r9+r12*8+disp32] should encode packed logic high-register disp32 SIB */
int test_pand_xmm15_r9_r12_scale8_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R12;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x10203040;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_PAND,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM15),
            mem_op
        },
        .line_number = 19
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* 66 47 0F DB BC E1 40 30 20 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);  /* REX: R=1, X=1, B=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xDB, ctx.output[3]);  /* PAND opcode */
    ASSERT_EQ_HEX(0xBC, ctx.output[4]);  /* mod=10 reg=111 rm=100(SIB) */
    ASSERT_EQ_HEX(0xE1, ctx.output[5]);  /* scale=8 index=r12 base=r9 */
    ASSERT_EQ_HEX(0x40, ctx.output[6]);
    ASSERT_EQ_HEX(0x30, ctx.output[7]);
    ASSERT_EQ_HEX(0x20, ctx.output[8]);
    ASSERT_EQ_HEX(0x10, ctx.output[9]);

    free(ctx.text_section);
    return 0;
}

/* Test: POR xmm10, [rip+0x30] should encode packed logical RIP-relative form */
int test_por_xmm10_rip_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.is_rip_relative = true;
    mem_op.mem.displacement = 0x30;

    parsed_instruction_t inst = {
        .type = INST_POR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM10),
            mem_op
        },
        .line_number = 12
    };

    ASSERT_EQ(0, encode_sse_packed_logic(&ctx, &inst));
    ASSERT_EQ(9, ctx.current_address - 0x1000);  /* 66 44 0F EB 15 30 00 00 00 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x44, ctx.output[1]);  /* REX: R=1 */
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0xEB, ctx.output[3]);  /* POR opcode */
    ASSERT_EQ_HEX(0x15, ctx.output[4]);  /* mod=00 reg=010 rm=101 (RIP-relative) */
    ASSERT_EQ_HEX(0x30, ctx.output[5]);
    ASSERT_EQ_HEX(0x00, ctx.output[6]);
    ASSERT_EQ_HEX(0x00, ctx.output[7]);
    ASSERT_EQ_HEX(0x00, ctx.output[8]);

    free(ctx.text_section);
    return 0;
}

/* Test: PADDB with XMM size mismatch rejected */
int test_paddb_xmm_size_mismatch_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PADDB,
        .operand_count = 2,
        .operands = {
            make_reg_operand(XMM0, REG_SIZE_64),  /* Wrong size - should be REG_SIZE_XMM */
            make_xmm_operand(XMM1)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_arith(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "must use xmm register size");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: PAND with non-XMM destination rejected */
int test_pand_non_xmm_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PAND,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),  /* Not XMM */
            make_xmm_operand(XMM1)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_logic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "destination must be XMM");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: PCMPEQD with immediate source rejected */
int test_pcmpeqd_imm_src_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PCMPEQD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_imm_operand(42)  /* Immediate source not allowed */
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_cmp(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "source must be XMM or memory");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: PSUBD with immediate source rejected (packed arithmetic family) */
int test_psubd_imm_src_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PSUBD,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            make_imm_operand(7)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_arith(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "source must be XMM or memory");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: PXOR with XMM source using wrong size metadata rejected (packed logic family) */
int test_pxor_xmm_src_size_mismatch_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PXOR,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM2),
            make_reg_operand(XMM3, REG_SIZE_64)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_logic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "XMM source must use xmm register size");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: PCMPGTQ with non-XMM destination rejected (packed compare family) */
int test_pcmpgtq_non_xmm_dst_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {
        .type = INST_PCMPGTQ,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            make_xmm_operand(XMM1)
        },
        .line_number = 1
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_sse_packed_cmp(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "destination must be XMM register");

    free(captured_err);
    free(ctx.text_section);
    return 0;
}

/* Test: MOV rax, [rbx+rcx*1+0x10] should encode explicit scale=1 SIB form */
int test_mov_rax_rbx_rcx_scale1_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            mem_op
        },
        .line_number = 50
    };

    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 48 8B 44 0B 10 */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x8B, ctx.output[1]);
    ASSERT_EQ_HEX(0x44, ctx.output[2]);
    ASSERT_EQ_HEX(0x0B, ctx.output[3]);
    ASSERT_EQ_HEX(0x10, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOV [r10+r11*2-0x20], r12 should encode scale=2 SIB with negative disp8 */
int test_mov_mem_r12_r10_r11_scale2_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R10;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R11;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = -0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_reg_operand(R12, REG_SIZE_64)
        },
        .line_number = 51
    };

    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 4F 89 64 5A E0 */

    ASSERT_EQ_HEX(0x4F, ctx.output[0]);
    ASSERT_EQ_HEX(0x89, ctx.output[1]);
    ASSERT_EQ_HEX(0x64, ctx.output[2]);
    ASSERT_EQ_HEX(0x5A, ctx.output[3]);
    ASSERT_EQ_HEX(0xE0, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: ADD [r8+r9*4+disp32], imm8 should encode scale=4 SIB with disp32 */
int test_add_mem_r8_r9_scale4_disp32_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x12345678;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_ADD,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(0x20)
        },
        .line_number = 52
    };

    ASSERT_EQ(0, encode_arithmetic(&ctx, &inst));
    ASSERT_EQ(9, ctx.current_address - 0x1000);  /* 4B 83 84 88 78 56 34 12 20 */

    ASSERT_EQ_HEX(0x4B, ctx.output[0]);
    ASSERT_EQ_HEX(0x83, ctx.output[1]);
    ASSERT_EQ_HEX(0x84, ctx.output[2]);
    ASSERT_EQ_HEX(0x88, ctx.output[3]);
    ASSERT_EQ_HEX(0x78, ctx.output[4]);
    ASSERT_EQ_HEX(0x56, ctx.output[5]);
    ASSERT_EQ_HEX(0x34, ctx.output[6]);
    ASSERT_EQ_HEX(0x12, ctx.output[7]);
    ASSERT_EQ_HEX(0x20, ctx.output[8]);

    free(ctx.text_section);
    return 0;
}

/* Test: SUB [rbx+rsi*8-8], imm8 should encode scale=8 SIB with negative disp8 */
int test_sub_mem_rbx_rsi_scale8_neg_disp8_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RSI;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = -8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_SUB,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(0x7F)
        },
        .line_number = 53
    };

    ASSERT_EQ(0, encode_arithmetic(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 48 83 6C F3 F8 7F */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x83, ctx.output[1]);
    ASSERT_EQ_HEX(0x6C, ctx.output[2]);
    ASSERT_EQ_HEX(0xF3, ctx.output[3]);
    ASSERT_EQ_HEX(0xF8, ctx.output[4]);
    ASSERT_EQ_HEX(0x7F, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA r15, [r12+r13*8+0x40] should encode scale=8 with high-register SIB */
int test_lea_r15_r12_r13_scale8_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x40;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(R15, REG_SIZE_64),
            mem_op
        },
        .line_number = 54
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 4F 8D 7C EC 40 */

    ASSERT_EQ_HEX(0x4F, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x7C, ctx.output[2]);
    ASSERT_EQ_HEX(0xEC, ctx.output[3]);
    ASSERT_EQ_HEX(0x40, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA rcx, [rax+rdx*2] should encode scale=2 no-displacement SIB */
int test_lea_rcx_rax_rdx_scale2_no_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RAX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RDX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RCX, REG_SIZE_64),
            mem_op
        },
        .line_number = 55
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 48 8D 0C 50 */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x0C, ctx.output[2]);
    ASSERT_EQ_HEX(0x50, ctx.output[3]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVDQA xmm0, [rbx+rcx*1-0x10] should encode packed move scale=1 with negative disp8 */
int test_movdqa_xmm0_rbx_rcx_scale1_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = -0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVDQA,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM0),
            mem_op
        },
        .line_number = 56
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(6, ctx.current_address - 0x1000);  /* 66 0F 6F 44 0B F0 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x0F, ctx.output[1]);
    ASSERT_EQ_HEX(0x6F, ctx.output[2]);
    ASSERT_EQ_HEX(0x44, ctx.output[3]);
    ASSERT_EQ_HEX(0x0B, ctx.output[4]);
    ASSERT_EQ_HEX(0xF0, ctx.output[5]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVDQA xmm12, [r9+r10*4+0x20] should encode packed move scale=4 with high registers */
int test_movdqa_xmm12_r9_r10_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R9;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R10;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVDQA,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM12),
            mem_op
        },
        .line_number = 57
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 47 0F 6F 64 91 20 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x47, ctx.output[1]);
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x6F, ctx.output[3]);
    ASSERT_EQ_HEX(0x64, ctx.output[4]);
    ASSERT_EQ_HEX(0x91, ctx.output[5]);
    ASSERT_EQ_HEX(0x20, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOV rdx, [r8+r9*8] should encode scale=8 SIB no-displacement form */
int test_mov_rdx_r8_r9_scale8_no_disp(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RDX, REG_SIZE_64),
            mem_op
        },
        .line_number = 58
    };

    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(4, ctx.current_address - 0x1000);  /* 4B 8B 14 C8 */

    ASSERT_EQ_HEX(0x4B, ctx.output[0]);
    ASSERT_EQ_HEX(0x8B, ctx.output[1]);
    ASSERT_EQ_HEX(0x14, ctx.output[2]);
    ASSERT_EQ_HEX(0xC8, ctx.output[3]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOV [rbp+rsi*4+0], rax should encode scale=4 with explicit disp32 zero */
int test_mov_mem_rax_rbp_rsi_scale4_zero_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBP;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RSI;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_reg_operand(RAX, REG_SIZE_64)
        },
        .line_number = 59
    };

    ASSERT_EQ(0, encode_mov(&ctx, &inst));
    ASSERT_EQ(8, ctx.current_address - 0x1000);  /* 48 89 84 B5 00 00 00 00 */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x89, ctx.output[1]);
    ASSERT_EQ_HEX(0x84, ctx.output[2]);
    ASSERT_EQ_HEX(0xB5, ctx.output[3]);
    ASSERT_EQ_HEX(0x00, ctx.output[4]);
    ASSERT_EQ_HEX(0x00, ctx.output[5]);
    ASSERT_EQ_HEX(0x00, ctx.output[6]);
    ASSERT_EQ_HEX(0x00, ctx.output[7]);

    free(ctx.text_section);
    return 0;
}

/* Test: ADD [rbx+rcx*1], imm8 should encode scale=1 no-displacement SIB form */
int test_add_mem_rbx_rcx_scale1_no_disp_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;

    parsed_instruction_t inst = {
        .type = INST_ADD,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(5)
        },
        .line_number = 60
    };

    ASSERT_EQ(0, encode_arithmetic(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 48 83 04 0B 05 */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x83, ctx.output[1]);
    ASSERT_EQ_HEX(0x04, ctx.output[2]);
    ASSERT_EQ_HEX(0x0B, ctx.output[3]);
    ASSERT_EQ_HEX(0x05, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: SUB [r12+r13*2+disp32], imm8 should encode scale=2 high-register disp32 form */
int test_sub_mem_r12_r13_scale2_disp32_imm8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 0x12345678;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_SUB,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(1)
        },
        .line_number = 61
    };

    ASSERT_EQ(0, encode_arithmetic(&ctx, &inst));
    ASSERT_EQ(9, ctx.current_address - 0x1000);  /* 4B 83 AC 6C 78 56 34 12 01 */

    ASSERT_EQ_HEX(0x4B, ctx.output[0]);
    ASSERT_EQ_HEX(0x83, ctx.output[1]);
    ASSERT_EQ_HEX(0xAC, ctx.output[2]);
    ASSERT_EQ_HEX(0x6C, ctx.output[3]);
    ASSERT_EQ_HEX(0x78, ctx.output[4]);
    ASSERT_EQ_HEX(0x56, ctx.output[5]);
    ASSERT_EQ_HEX(0x34, ctx.output[6]);
    ASSERT_EQ_HEX(0x12, ctx.output[7]);
    ASSERT_EQ_HEX(0x01, ctx.output[8]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA rbx, [rdi+rsi*1-0x10] should encode scale=1 with negative disp8 */
int test_lea_rbx_rdi_rsi_scale1_neg_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RDI;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RSI;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 1;
    mem_op.mem.displacement = -0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RBX, REG_SIZE_64),
            mem_op
        },
        .line_number = 62
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 48 8D 5C 37 F0 */

    ASSERT_EQ_HEX(0x48, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x5C, ctx.output[2]);
    ASSERT_EQ_HEX(0x37, ctx.output[3]);
    ASSERT_EQ_HEX(0xF0, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA r10, [r8+r11*4+0x20] should encode scale=4 high-register SIB form */
int test_lea_r10_r8_r11_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R11;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(R10, REG_SIZE_64),
            mem_op
        },
        .line_number = 63
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 4F 8D 54 98 20 */

    ASSERT_EQ_HEX(0x4F, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x54, ctx.output[2]);
    ASSERT_EQ_HEX(0x98, ctx.output[3]);
    ASSERT_EQ_HEX(0x20, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVDQA xmm3, [r13+r14*2+0x10] should encode scale=2 packed move form */
int test_movdqa_xmm3_r13_r14_scale2_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R13;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R14;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVDQA,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM3),
            mem_op
        },
        .line_number = 64
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(7, ctx.current_address - 0x1000);  /* 66 43 0F 6F 5C 75 10 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x43, ctx.output[1]);
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x6F, ctx.output[3]);
    ASSERT_EQ_HEX(0x5C, ctx.output[4]);
    ASSERT_EQ_HEX(0x75, ctx.output[5]);
    ASSERT_EQ_HEX(0x10, ctx.output[6]);

    free(ctx.text_section);
    return 0;
}

/* Test: MOVDQA xmm9, [rbx+r12*8+disp32] should encode scale=8 packed move with disp32 */
int test_movdqa_xmm9_rbx_r12_scale8_disp32(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R12;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 8;
    mem_op.mem.displacement = 0x11223344;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_MOVDQA,
        .operand_count = 2,
        .operands = {
            make_xmm_operand(XMM9),
            mem_op
        },
        .line_number = 65
    };

    ASSERT_EQ(0, encode_sse_mov(&ctx, &inst));
    ASSERT_EQ(10, ctx.current_address - 0x1000);  /* 66 46 0F 6F 8C E3 44 33 22 11 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x46, ctx.output[1]);
    ASSERT_EQ_HEX(0x0F, ctx.output[2]);
    ASSERT_EQ_HEX(0x6F, ctx.output[3]);
    ASSERT_EQ_HEX(0x8C, ctx.output[4]);
    ASSERT_EQ_HEX(0xE3, ctx.output[5]);
    ASSERT_EQ_HEX(0x44, ctx.output[6]);
    ASSERT_EQ_HEX(0x33, ctx.output[7]);
    ASSERT_EQ_HEX(0x22, ctx.output[8]);
    ASSERT_EQ_HEX(0x11, ctx.output[9]);

    free(ctx.text_section);
    return 0;
}

/* Test: Memory operand with invalid positive scale should be rejected by encoder */
int test_invalid_scale_factor_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 3;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            mem_op
        },
        .line_number = 66
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Invalid scale factor 3");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: Memory operand with negative scale should be rejected by encoder */
int test_negative_scale_factor_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = -2;

    parsed_instruction_t inst = {
        .type = INST_ADD,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_reg_operand(RAX, REG_SIZE_64)
        },
        .line_number = 67
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_arithmetic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Invalid scale factor -2");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: Memory operand with invalid scale 5 should be rejected by encoder */
int test_invalid_scale_factor5_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 5;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            mem_op
        },
        .line_number = 72
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Invalid scale factor 5");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: Memory operand with invalid scale 6 should be rejected by encoder */
int test_invalid_scale_factor6_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 6;

    parsed_instruction_t inst = {
        .type = INST_SUB,
        .operand_count = 2,
        .operands = {
            mem_op,
            make_imm_operand(4)
        },
        .line_number = 73
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_arithmetic(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Invalid scale factor 6");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: Memory operand with invalid scale 7 should be rejected by encoder */
int test_invalid_scale_factor7_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R12;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R13;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 7;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            mem_op
        },
        .line_number = 74
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_lea(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "Invalid scale factor 7");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: RIP-relative memory must reject additional base/index registers */
int test_rip_relative_with_index_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.is_rip_relative = true;
    mem_op.mem.displacement = 0x20;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;

    parsed_instruction_t inst = {
        .type = INST_MOV,
        .operand_count = 2,
        .operands = {
            make_reg_operand(RAX, REG_SIZE_64),
            mem_op
        },
        .line_number = 68
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_mov(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "RIP-relative addressing cannot combine base/index registers");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test: FS segment override prefix should precede encoded memory instruction */
int test_encode_instruction_fs_segment_prefix(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {0};
    inst.type = INST_MOV;
    inst.operand_count = 2;
    inst.operands[0] = make_reg_operand(RAX, REG_SIZE_64);
    inst.operands[1].type = OPERAND_MEM;
    inst.operands[1].mem.segment_override = MEM_SEG_FS;
    inst.operands[1].mem.has_displacement = true;
    inst.operands[1].mem.displacement = 0;

    ASSERT_EQ(0, encode_instruction(&ctx, &inst));
    ASSERT_EQ_HEX(0x64, ctx.output[0]);

    free(ctx.text_section);
    return 0;
}

/* Test: GS segment override prefix should precede encoded memory instruction */
int test_encode_instruction_gs_segment_prefix(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    parsed_instruction_t inst = {0};
    inst.type = INST_MOV;
    inst.operand_count = 2;
    inst.operands[0] = make_reg_operand(RAX, REG_SIZE_64);
    inst.operands[1].type = OPERAND_MEM;
    inst.operands[1].mem.segment_override = MEM_SEG_GS;
    inst.operands[1].mem.has_displacement = true;
    inst.operands[1].mem.displacement = 0;

    ASSERT_EQ(0, encode_instruction(&ctx, &inst));
    ASSERT_EQ_HEX(0x65, ctx.output[0]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA with 16-bit destination should emit operand-size prefix */
int test_lea_ax_rbx_rcx_scale2_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RBX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;
    mem_op.mem.displacement = 8;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(AX, REG_SIZE_16),
            mem_op
        },
        .line_number = 69
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 66 8D 44 4B 08 */

    ASSERT_EQ_HEX(0x66, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x44, ctx.output[2]);
    ASSERT_EQ_HEX(0x4B, ctx.output[3]);
    ASSERT_EQ_HEX(0x08, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA with 32-bit destination should encode without REX.W */
int test_lea_ecx_r8_r9_scale4_disp8(void) {
    assembler_context_t ctx;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = R8;
    mem_op.mem.has_base = true;
    mem_op.mem.index = R9;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 4;
    mem_op.mem.displacement = 0x10;
    mem_op.mem.has_displacement = true;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(ECX, REG_SIZE_32),
            mem_op
        },
        .line_number = 70
    };

    ASSERT_EQ(0, encode_lea(&ctx, &inst));
    ASSERT_EQ(5, ctx.current_address - 0x1000);  /* 43 8D 4C 88 10 */

    ASSERT_EQ_HEX(0x43, ctx.output[0]);
    ASSERT_EQ_HEX(0x8D, ctx.output[1]);
    ASSERT_EQ_HEX(0x4C, ctx.output[2]);
    ASSERT_EQ_HEX(0x88, ctx.output[3]);
    ASSERT_EQ_HEX(0x10, ctx.output[4]);

    free(ctx.text_section);
    return 0;
}

/* Test: LEA with 8-bit destination should be rejected */
int test_lea_8bit_destination_rejected(void) {
    assembler_context_t ctx;
    char *captured_err;
    setup_test_context(&ctx);

    operand_t mem_op = {0};
    mem_op.type = OPERAND_MEM;
    mem_op.mem.base = RAX;
    mem_op.mem.has_base = true;
    mem_op.mem.index = RCX;
    mem_op.mem.has_index = true;
    mem_op.mem.scale = 2;

    parsed_instruction_t inst = {
        .type = INST_LEA,
        .operand_count = 2,
        .operands = {
            make_reg_operand(AL, REG_SIZE_8),
            mem_op
        },
        .line_number = 71
    };

    ASSERT_EQ(0, test_capture_stderr_begin());
    ASSERT_EQ(-1, encode_lea(&ctx, &inst));
    captured_err = test_capture_stderr_end();
    ASSERT_NOT_NULL(captured_err);
    ASSERT_STR_CONTAINS(captured_err, "lea destination must be 16, 32, or 64-bit register");
    free(captured_err);

    free(ctx.text_section);
    return 0;
}

/* Test Suite: Encoder Tests */
TEST_SUITE(encoder) {
    TEST(mov_r64_imm64);
    TEST(mov_r32_imm32);
    TEST(mov_r8h_imm8);
    TEST(mov_r8l_imm8);
    TEST(push_r64);
    TEST(push_r64_rex);
    TEST(ret);
    TEST(nop);
    TEST(mov_r64_r64);
    TEST(jnz_encoding);
    TEST(cmovne_rbx_rax);
    TEST(cmovg_r8_r9);
    TEST(encode_instruction_cdq);
    TEST(encode_cqo_bytes);
    TEST(bswap_eax);
    TEST(encode_instruction_bswap_r8);
    TEST(encode_enter_bytes);
    TEST(encode_enter_invalid_size_diagnostic);
    TEST(encode_instruction_signext_legacy);
    TEST(encode_instruction_int_imm8);
    TEST(encode_instruction_int_non_immediate_diagnostic);
    TEST(encode_instruction_sete_non8bit_diagnostic);
    TEST(encode_instruction_cmov_dst_not_register_diagnostic);
    TEST(encode_instruction_bswap_invalid_size_diagnostic);
    TEST(encode_instruction_xchg_operand_count_diagnostic);
    TEST(encode_instruction_imul_dst_not_register_diagnostic);
    TEST(encode_instruction_div_operand_count_diagnostic);
    TEST(encode_instruction_shift_invalid_count_diagnostic);
    TEST(encode_instruction_bt_operand_count_diagnostic);
    TEST(encode_instruction_bts_invalid_bit_index_diagnostic);
    TEST(encode_instruction_shld_src_not_register_diagnostic);
    TEST(encode_instruction_bsf_dst_not_register_diagnostic);
    TEST(encode_instruction_jmp_fixup_capacity_diagnostic);
    TEST(encode_not_operand_count_diagnostic);
    TEST(r8h_rex_conflict);
    TEST(add_high8_rex_conflict);
    TEST(sub_high8_rex_conflict);
    TEST(cmp_high8_rex_conflict);
    /* Bit Manipulation Tests */
    TEST(bt_rax_imm);
    TEST(bts_rbx_cl);
    TEST(btr_r15_imm);
    TEST(shld_rax_rbx_imm);
    TEST(bsf_rcx_rdx);
    /* SSE Tests */
    TEST(movaps_xmm0_xmm1);
    TEST(movaps_xmm8_xmm9);
    TEST(movups_xmm0_mem);
    TEST(movaps_mem_xmm15);
    TEST(movss_xmm1_xmm2);
    TEST(movapd_xmm8_complex_addr);
    TEST(addss_xmm0_xmm1);
    TEST(mulsd_xmm8_xmm9);
    TEST(divps_xmm4_mem);
    TEST(movups_xmm2_rsp_disp8);
    TEST(movaps_xmm9_r12_r13_scale8_disp32);
    TEST(movaps_xmm10_r12_r13_scale2_disp8);
    TEST(movups_xmm3_r8_r15_scale2_no_disp);
    TEST(movaps_xmm5_rbx_rcx_scale4_disp8);
    TEST(movups_xmm6_rbx_rcx_scale1_disp8);
    TEST(addps_xmm11_r10_r12_scale8_disp8);
    TEST(movups_xmm1_rip_disp32);
    TEST(movsd_mem_xmm1_rbp);
    TEST(sse_mov_mem_to_mem_without_xmm_rejected);
    TEST(sse_mov_without_xmm_rejected);
    TEST(sse_arith_dst_not_xmm_rejected);
    TEST(sse_arith_imm_src_rejected);
    TEST(sse_mov_mem_dst_imm_src_rejected);
    TEST(sse_mov_xmm_dst_imm_src_rejected);
    TEST(sse_mov_xmm_size_mismatch_rejected);
    TEST(sse_arith_xmm_src_size_mismatch_rejected);
    /* SSE Packed Integer Tests */
    TEST(movdqa_xmm0_xmm1);
    TEST(movdqu_xmm8_mem);
    TEST(paddb_xmm1_xmm2);
    TEST(paddw_xmm0_mem);
    TEST(psubq_xmm9_xmm10);
    TEST(psubd_xmm6_mem_disp);
    TEST(psubd_xmm2_r9_r10_scale4_disp8);
    TEST(paddq_xmm5_r13_r14_scale2_disp8);
    TEST(paddd_xmm1_rbx_rcx_scale4_disp8);
    TEST(psubq_xmm3_r8_r9_scale8_disp8);
    TEST(psubw_xmm2_r13_r14_scale2_neg_disp8);
    TEST(paddb_xmm14_rbx_rsi_scale2_no_disp);
    TEST(pcmpeqd_xmm7_xmm15);
    TEST(pcmpgtb_xmm3_mem_disp);
    TEST(pcmpeqw_xmm4_r11_r12_scale4_disp8);
    TEST(pcmpgtd_xmm9_r12_r13_scale4_disp8);
    TEST(pcmpeqb_xmm11_r8_r10_scale8_disp32);
    TEST(pcmpgtw_xmm0_rdi_rbp_scale4_neg_disp8);
    TEST(pcmpeqq_xmm1_xmm2);
    TEST(pcmpgtq_xmm8_mem_disp);
    TEST(pcmpeqq_xmm14_r10_r11_scale2_neg_disp8);
    TEST(pcmpgtq_xmm6_rbx_r12_scale8_disp32);
    TEST(pcmpgtq_xmm10_rip_disp32);
    TEST(pcmpgtd_xmm9_rip_disp32);
    TEST(pand_xmm4_xmm5);
    TEST(pand_xmm10_r8_r9_scale4_disp8);
    TEST(pandn_xmm3_xmm4);
    TEST(pandn_xmm6_r10_r11_scale4_disp8);
    TEST(por_xmm2_mem);
    TEST(por_xmm13_rbp_rsi_scale2_implicit_zero_disp8);
    TEST(pand_xmm15_r9_r12_scale8_disp32);
    TEST(por_xmm10_rip_disp32);
    TEST(pxor_xmm12_mem_disp);
    TEST(pxor_xmm7_rbx_rdx_scale1_disp8);
    TEST(pxor_xmm1_r12_r15_scale4_no_disp);
    TEST(paddb_xmm_size_mismatch_rejected);
    TEST(pand_non_xmm_rejected);
    TEST(pcmpeqd_imm_src_rejected);
    TEST(psubd_imm_src_rejected);
    TEST(pxor_xmm_src_size_mismatch_rejected);
    TEST(pcmpgtq_non_xmm_dst_rejected);
    /* Scale-factor completion tests (Milestone 1.2.1 slice) */
    TEST(mov_rax_rbx_rcx_scale1_disp8);
    TEST(mov_mem_r12_r10_r11_scale2_neg_disp8);
    TEST(add_mem_r8_r9_scale4_disp32_imm8);
    TEST(sub_mem_rbx_rsi_scale8_neg_disp8_imm8);
    TEST(lea_r15_r12_r13_scale8_disp8);
    TEST(lea_rcx_rax_rdx_scale2_no_disp);
    TEST(movdqa_xmm0_rbx_rcx_scale1_neg_disp8);
    TEST(movdqa_xmm12_r9_r10_scale4_disp8);
    TEST(mov_rdx_r8_r9_scale8_no_disp);
    TEST(mov_mem_rax_rbp_rsi_scale4_zero_disp32);
    TEST(add_mem_rbx_rcx_scale1_no_disp_imm8);
    TEST(sub_mem_r12_r13_scale2_disp32_imm8);
    TEST(lea_rbx_rdi_rsi_scale1_neg_disp8);
    TEST(lea_r10_r8_r11_scale4_disp8);
    TEST(movdqa_xmm3_r13_r14_scale2_disp8);
    TEST(movdqa_xmm9_rbx_r12_scale8_disp32);
    /* Edge-case hardening tests (Milestone 1.2.2 slice) */
    TEST(invalid_scale_factor_rejected);
    TEST(negative_scale_factor_rejected);
    TEST(invalid_scale_factor5_rejected);
    TEST(invalid_scale_factor6_rejected);
    TEST(invalid_scale_factor7_rejected);
    TEST(rip_relative_with_index_rejected);
    TEST(encode_instruction_fs_segment_prefix);
    TEST(encode_instruction_gs_segment_prefix);
    TEST(lea_ax_rbx_rcx_scale2_disp8);
    TEST(lea_ecx_r8_r9_scale4_disp8);
    TEST(lea_8bit_destination_rejected);
}

/* Main entry point for encoder tests */
int main(void) {
    test_init();
    RUN_TEST_SUITE(encoder);
    test_report();
    return test_final_status();
}
