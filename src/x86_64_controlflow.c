/**
 * x86_64 Control Flow Encoding - Jumps, Calls, Returns
 */

#include "x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* External emit functions */
extern int emit_byte(assembler_context_t *ctx, uint8_t byte);
extern int emit_le32(assembler_context_t *ctx, int32_t val);
extern int emit_le16(assembler_context_t *ctx, int16_t val);
extern int emit_rex(assembler_context_t *ctx, bool w, const operand_t *reg_op,
                    const operand_t *rm_op, const operand_t *sib_index_op);
extern int emit_modrm_sib(assembler_context_t *ctx, uint8_t reg_opcode,
                          const operand_t *operand, reg_size_t size);

/* Add a fixup for forward reference */
static int add_fixup(assembler_context_t *ctx, const char *label,
                     int size, instruction_type_t inst_type) {
    if (ctx->fixup_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Too many fixups\n");
        return -1;
    }

    strncpy(ctx->fixups[ctx->fixup_count].label, label, MAX_LABEL_LENGTH - 1);
    ctx->fixups[ctx->fixup_count].label[MAX_LABEL_LENGTH - 1] = '\0';
    ctx->fixups[ctx->fixup_count].location = ctx->current_address;
    ctx->fixups[ctx->fixup_count].size = size;
    ctx->fixups[ctx->fixup_count].instruction_type = inst_type;
    ctx->fixups[ctx->fixup_count].operand_index = 0;

    ctx->fixup_count++;
    return 0;
}

/* JMP encoding */
int encode_jmp(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: jmp requires 1 operand at line %d\n", inst->line_number);
        return -1;
    }

    const operand_t *op = &inst->operands[0];

    /* jmp label - relative jump (always use fixup for correctness) */
    if (op->type == OPERAND_LABEL) {
        if (emit_byte(ctx, 0xE9) < 0) return -1;
        if (add_fixup(ctx, op->label, 4, INST_JMP) < 0) return -1;
        if (emit_le32(ctx, 0) < 0) return -1; /* Placeholder */
        return 0;
    }

    /* jmp reg - indirect jump */
    if (op->type == OPERAND_REG) {
        /* FF /4 - JMP r/m64 */
        if (emit_rex(ctx, true, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, 0xFF) < 0) return -1;
        if (emit_modrm_sib(ctx, 4, op, REG_SIZE_64) < 0) return -1;
        return 0;
    }

    /* jmp mem - indirect jump */
    if (op->type == OPERAND_MEM) {
        if (emit_rex(ctx, true, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, 0xFF) < 0) return -1;
        if (emit_modrm_sib(ctx, 4, op, REG_SIZE_64) < 0) return -1;
        return 0;
    }

    fprintf(stderr, "Error: Unsupported jmp operand at line %d\n", inst->line_number);
    return -1;
}

/* Conditional jumps encoding */
int encode_jcc(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: Conditional jump requires 1 operand at line %d\n", inst->line_number);
        return -1;
    }

    const operand_t *op = &inst->operands[0];

    if (op->type != OPERAND_LABEL) {
        fprintf(stderr, "Error: Conditional jump requires label operand\n");
        return -1;
    }

    /* Get condition code */
    uint8_t cc;
    switch (inst->type) {
        case INST_JA:  cc = 0x7; break;  /* Above (CF=0 && ZF=0) */
        case INST_JAE: cc = 0x3; break;  /* Above or Equal (CF=0) */
        case INST_JB:  cc = 0x2; break;  /* Below (CF=1) */
        case INST_JBE: cc = 0x6; break;  /* Below or Equal (CF=1 || ZF=1) */
        case INST_JE:  cc = 0x4; break;  /* Equal (ZF=1) */
        case INST_JG:  cc = 0xF; break;  /* Greater (ZF=0 && SF=OF) */
        case INST_JGE: cc = 0xD; break;  /* Greater or Equal (SF=OF) */
        case INST_JL:  cc = 0xC; break;  /* Less (SF!=OF) */
        case INST_JLE: cc = 0xE; break;  /* Less or Equal (ZF=1 || SF!=OF) */
        case INST_JNE: cc = 0x5; break;  /* Not Equal (ZF=0) */
        case INST_JNZ: cc = 0x5; break;  /* Not Zero - synonym for JNE */
        case INST_JNO: cc = 0x1; break;  /* Not Overflow (OF=0) */
        case INST_JNP: cc = 0xB; break;  /* Not Parity (PF=0) */
        case INST_JNS: cc = 0x9; break;  /* Not Sign (SF=0) */
        case INST_JO:  cc = 0x0; break;  /* Overflow (OF=1) */
        case INST_JP:  cc = 0xA; break;  /* Parity (PF=1) */
        case INST_JS:  cc = 0x8; break;  /* Sign (SF=1) */
        default:
            fprintf(stderr, "Error: Unknown conditional jump\n");
            return -1;
    }

    /* Always use fixups for control flow to ensure correct addresses.
     * Symbol addresses from first pass may be wrong due to estimation.
     * Fixups are resolved in third pass when all addresses are final. */

    /* Near jump: 0F 80+cc cd */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, 0x80 + cc) < 0) return -1;
    if (add_fixup(ctx, op->label, 4, inst->type) < 0) return -1;
    if (emit_le32(ctx, 0) < 0) return -1; /* Placeholder */

    return 0;
}

/* CALL encoding */
int encode_call_ret(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->type == INST_RET) {
        if (inst->operand_count == 0) {
            /* Near return: C3 */
            if (emit_byte(ctx, 0xC3) < 0) return -1;
            return 0;
        } else if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_IMM) {
            /* Near return with pop: C2 iw */
            if (emit_byte(ctx, 0xC2) < 0) return -1;
            if (emit_le16(ctx, (int16_t)inst->operands[0].immediate) < 0) return -1;
            return 0;
        }
        fprintf(stderr, "Error: ret with invalid operands\n");
        return -1;
    }

    if (inst->type == INST_CALL) {
        if (inst->operand_count != 1) {
            fprintf(stderr, "Error: call requires 1 operand at line %d\n", inst->line_number);
            return -1;
        }

        const operand_t *op = &inst->operands[0];

        /* call label - relative call (always use fixup for correctness) */
        if (op->type == OPERAND_LABEL) {
            if (emit_byte(ctx, 0xE8) < 0) return -1;
            if (add_fixup(ctx, op->label, 4, INST_CALL) < 0) return -1;
            if (emit_le32(ctx, 0) < 0) return -1;
            return 0;
        }

        /* call reg - indirect call */
        if (op->type == OPERAND_REG) {
            /* FF /2 - CALL r/m64 */
            if (emit_rex(ctx, true, NULL, op, NULL) < 0) return -1;
            if (emit_byte(ctx, 0xFF) < 0) return -1;
            if (emit_modrm_sib(ctx, 2, op, REG_SIZE_64) < 0) return -1;
            return 0;
        }

        /* call mem - indirect call */
        if (op->type == OPERAND_MEM) {
            if (emit_rex(ctx, true, NULL, op, NULL) < 0) return -1;
            if (emit_byte(ctx, 0xFF) < 0) return -1;
            if (emit_modrm_sib(ctx, 2, op, REG_SIZE_64) < 0) return -1;
            return 0;
        }
    }

    fprintf(stderr, "Error: Unsupported call/ret at line %d\n", inst->line_number);
    return -1;
}

/* Syscall/sysret encoding */
int encode_syscall(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    /* SYSCALL: 0F 05 */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, 0x05) < 0) return -1;
    return 0;
}

int encode_sysret(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    /* SYSRET: 0F 07 */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, 0x07) < 0) return -1;
    return 0;
}

/* NOP encoding */
int encode_nop(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count == 0) {
        /* Single byte NOP: 90 */
        if (emit_byte(ctx, 0x90) < 0) return -1;
        return 0;
    }
    /* Multi-byte NOP can be added later */
    fprintf(stderr, "Error: nop with operands not supported\n");
    return -1;
}

/* HLT encoding */
int encode_hlt(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    if (emit_byte(ctx, 0xF4) < 0) return -1;
    return 0;
}

/* INT 0x80 encoding (for Linux 32-bit compatibility) */
int encode_int(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1 || inst->operands[0].type != OPERAND_IMM) {
        fprintf(stderr, "Error: int requires immediate operand\n");
        return -1;
    }

    uint8_t vec = (uint8_t)inst->operands[0].immediate;
    if (emit_byte(ctx, 0xCD) < 0) return -1;
    if (emit_byte(ctx, vec) < 0) return -1;
    return 0;
}

/* CDQ/CQO encoding */
int encode_cwd_cdq_cqo(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    /* CQO: REX.W + 99 */
    if (emit_byte(ctx, 0x48) < 0) return -1; /* REX.W */
    if (emit_byte(ctx, 0x99) < 0) return -1;
    return 0;
}

/* CWD/CWDE/CDQE encoding */
int encode_cwd(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    /* CDQ: 99 (implicitly 32-bit in 32-bit mode, but we need REX.W for 64-bit) */
    if (emit_byte(ctx, 0x99) < 0) return -1;
    return 0;
}

int encode_cbw_cwde_cdqe(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    /* CDQE: REX.W + 98 */
    if (emit_byte(ctx, 0x48) < 0) return -1; /* REX.W */
    if (emit_byte(ctx, 0x98) < 0) return -1;
    return 0;
}

/* Flag operations */
int encode_clc_stc_cmc(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    switch (inst->type) {
        case INST_CLC: if (emit_byte(ctx, 0xF8) < 0) return -1; break;
        case INST_STC: if (emit_byte(ctx, 0xF9) < 0) return -1; break;
        case INST_CMC: if (emit_byte(ctx, 0xF5) < 0) return -1; break;
        default: return -1;
    }
    return 0;
}

int encode_cld_std(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    switch (inst->type) {
        case INST_CLD: if (emit_byte(ctx, 0xFC) < 0) return -1; break;
        case INST_STD: if (emit_byte(ctx, 0xFD) < 0) return -1; break;
        default: return -1;
    }
    return 0;
}

int encode_cli_sti(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    switch (inst->type) {
        case INST_CLI: if (emit_byte(ctx, 0xFA) < 0) return -1; break;
        case INST_STI: if (emit_byte(ctx, 0xFB) < 0) return -1; break;
        default: return -1;
    }
    return 0;
}

/* LEAVE encoding */
int encode_leave(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    (void)inst;
    if (emit_byte(ctx, 0xC9) < 0) return -1;
    return 0;
}

/* SETcc encoding */
int encode_setcc(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: setcc requires 1 operand\n");
        return -1;
    }

    const operand_t *op = &inst->operands[0];

    if (op->type != OPERAND_REG && op->type != OPERAND_MEM) {
        fprintf(stderr, "Error: setcc requires register or memory operand\n");
        return -1;
    }

    if (op->type == OPERAND_REG && op->reg_size != REG_SIZE_8 && op->reg_size != REG_SIZE_8H) {
        fprintf(stderr, "Error: setcc requires 8-bit operand\n");
        return -1;
    }

    uint8_t cc;
    switch (inst->type) {
        case INST_SETA:  cc = 0x7; break;
        case INST_SETAE: cc = 0x3; break;
        case INST_SETB:  cc = 0x2; break;
        case INST_SETBE: cc = 0x6; break;
        case INST_SETE:  cc = 0x4; break;
        case INST_SETG:  cc = 0xF; break;
        case INST_SETGE: cc = 0xD; break;
        case INST_SETL:  cc = 0xC; break;
        case INST_SETLE: cc = 0xE; break;
        case INST_SETNE: cc = 0x5; break;
        case INST_SETNO: cc = 0x1; break;
        case INST_SETNP: cc = 0xB; break;
        case INST_SETNS: cc = 0x9; break;
        case INST_SETO:  cc = 0x0; break;
        case INST_SETP:  cc = 0xA; break;
        case INST_SETS:  cc = 0x8; break;
        default: return -1;
    }

    /* 0F 90+cc - SETcc r/m8 */
    if (emit_rex(ctx, false, NULL, op, NULL) < 0) return -1;
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, 0x90 + cc) < 0) return -1;
    if (emit_modrm_sib(ctx, 0, op, REG_SIZE_8) < 0) return -1;

    return 0;
}

/* XCHG encoding */
int encode_xchg(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: xchg requires 2 operands\n");
        return -1;
    }

    const operand_t *op1 = &inst->operands[0];
    const operand_t *op2 = &inst->operands[1];

    /* XCHG rax, reg and XCHG reg, rax - special short form */
    if ((op1->type == OPERAND_REG && op1->reg == 0 && op2->type == OPERAND_REG) ||
        (op2->type == OPERAND_REG && op2->reg == 0 && op1->type == OPERAND_REG)) {
        const operand_t *reg_op = (op1->type == OPERAND_REG && op1->reg == 0) ? op2 : op1;
        reg_size_t size = reg_op->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        /* 90+rq - XCHG rAX, r64 */
        if (emit_rex(ctx, is_64bit, NULL, reg_op, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x90 + (reg_op->reg & 7)) < 0) return -1;
        return 0;
    }

    /* General case: XCHG r/m64, r64 */
    if (op1->type == OPERAND_REG && op2->type == OPERAND_REG) {
        reg_size_t size = op1->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        /* 87 /r - XCHG r/m64, r64 */
        if (emit_rex(ctx, is_64bit, op2, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x87) < 0) return -1;
        if (emit_modrm_sib(ctx, op2->reg & 7, op1, size) < 0) return -1;
        return 0;
    }

    /* XCHG reg, mem or XCHG mem, reg */
    if ((op1->type == OPERAND_REG && op2->type == OPERAND_MEM) ||
        (op1->type == OPERAND_MEM && op2->type == OPERAND_REG)) {
        const operand_t *reg_op = (op1->type == OPERAND_REG) ? op1 : op2;
        const operand_t *mem_op = (op1->type == OPERAND_MEM) ? op1 : op2;
        reg_size_t size = reg_op->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        if (emit_rex(ctx, is_64bit, reg_op, mem_op, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x87) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_op->reg & 7, mem_op, size) < 0) return -1;
        return 0;
    }

    fprintf(stderr, "Error: Unsupported xchg operands\n");
    return -1;
}

/* IMUL encoding */
int encode_imul(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    /* IMUL has multiple forms:
     * 1 operand:  IMUL r/m64 (signed multiply RAX by r/m64)
     * 2 operands: IMUL r64, r/m64
     * 3 operands: IMUL r64, r/m64, imm
     */

    if (inst->operand_count == 1) {
        /* F7 /5 - IMUL r/m64 */
        const operand_t *op = &inst->operands[0];
        reg_size_t size = REG_SIZE_64;
        if (op->type == OPERAND_REG) size = op->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        if (emit_rex(ctx, is_64bit, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, 0xF7) < 0) return -1;
        if (emit_modrm_sib(ctx, 5, op, size) < 0) return -1;
        return 0;
    }

    if (inst->operand_count == 2) {
        /* 0F AF /r - IMUL r64, r/m64 */
        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];

        if (dst->type != OPERAND_REG) {
            fprintf(stderr, "Error: IMUL destination must be register\n");
            return -1;
        }

        reg_size_t size = dst->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x0F) < 0) return -1;
        if (emit_byte(ctx, 0xAF) < 0) return -1;
        if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
        return 0;
    }

    if (inst->operand_count == 3) {
        /* 6B /r ib or 69 /r id - IMUL r64, r/m64, imm */
        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        const operand_t *imm = &inst->operands[2];

        if (dst->type != OPERAND_REG || imm->type != OPERAND_IMM) {
            fprintf(stderr, "Error: IMUL 3-operand form requires reg, reg/mem, imm\n");
            return -1;
        }

        reg_size_t size = dst->reg_size;
        bool is_64bit = (size == REG_SIZE_64);
        int32_t imm_val = (int32_t)imm->immediate;

        if (imm_val >= -128 && imm_val <= 127) {
            /* 6B /r ib */
            if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x6B) < 0) return -1;
            if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)imm_val) < 0) return -1;
        } else {
            /* 69 /r id */
            if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x69) < 0) return -1;
            if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
            if (emit_le32(ctx, imm_val) < 0) return -1;
        }
        return 0;
    }

    fprintf(stderr, "Error: IMUL requires 1, 2, or 3 operands\n");
    return -1;
}

/* DIV/IDIV/MUL encoding */
int encode_div_idiv_mul(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: div/idiv/mul requires 1 operand\n");
        return -1;
    }

    const operand_t *op = &inst->operands[0];
    reg_size_t size = REG_SIZE_64;
    if (op->type == OPERAND_REG) size = op->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    uint8_t reg_opcode;
    uint8_t opcode;

    switch (inst->type) {
        case INST_MUL:
            reg_opcode = 4;
            opcode = 0xF7;
            break;
        case INST_DIV:
            reg_opcode = 6;
            opcode = 0xF7;
            break;
        case INST_IDIV:
            reg_opcode = 7;
            opcode = 0xF7;
            break;
        default:
            fprintf(stderr, "Error: Unknown multiply/divide instruction\n");
            return -1;
    }

    if (emit_rex(ctx, is_64bit, NULL, op, NULL) < 0) return -1;
    if (emit_byte(ctx, opcode) < 0) return -1;
    if (emit_modrm_sib(ctx, reg_opcode, op, size) < 0) return -1;

    return 0;
}

/* TEST encoding */
int encode_test(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: test requires 2 operands\n");
        return -1;
    }

    const operand_t *op1 = &inst->operands[0];
    const operand_t *op2 = &inst->operands[1];

    /* TEST r/m64, imm32 - sign-extended immediate */
    if ((op1->type == OPERAND_REG || op1->type == OPERAND_MEM) && op2->type == OPERAND_IMM) {
        reg_size_t size = REG_SIZE_64;
        if (op1->type == OPERAND_REG) size = op1->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        /* Special case: TEST AL/AX/EAX/RAX, imm - shorter encoding */
        if (op1->type == OPERAND_REG && op1->reg == 0 && size != REG_SIZE_8) {
            /* A9 id - TEST rAX, imm32 */
            if (emit_rex(ctx, is_64bit, NULL, NULL, NULL) < 0) return -1;
            if (emit_byte(ctx, 0xA9) < 0) return -1;
            if (emit_le32(ctx, (int32_t)op2->immediate) < 0) return -1;
            return 0;
        }

        if (size == REG_SIZE_8) {
            /* F6 /0 ib - TEST r/m8, imm8 */
            if (emit_rex(ctx, false, NULL, op1, NULL) < 0) return -1;
            if (emit_byte(ctx, 0xF6) < 0) return -1;
            if (emit_modrm_sib(ctx, 0, op1, size) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)op2->immediate) < 0) return -1;
        } else {
            /* F7 /0 id - TEST r/m64, imm32 */
            if (emit_rex(ctx, is_64bit, NULL, op1, NULL) < 0) return -1;
            if (emit_byte(ctx, 0xF7) < 0) return -1;
            if (emit_modrm_sib(ctx, 0, op1, size) < 0) return -1;
            if (emit_le32(ctx, (int32_t)op2->immediate) < 0) return -1;
        }
        return 0;
    }

    /* TEST r/m64, r64 */
    if (op2->type == OPERAND_REG) {
        reg_size_t size = op2->reg_size;
        bool is_64bit = (size == REG_SIZE_64);

        /* 85 /r - TEST r/m64, r64 */
        if (emit_rex(ctx, is_64bit, op2, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x85) < 0) return -1;
        if (emit_modrm_sib(ctx, op2->reg & 7, op1, size) < 0) return -1;
        return 0;
    }

    fprintf(stderr, "Error: Unsupported test operands\n");
    return -1;
}

/* NOT encoding (already in encode_unary_arithmetic, but separate for clarity) */
int encode_not(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: not requires 1 operand\n");
        return -1;
    }

    const operand_t *op = &inst->operands[0];
    reg_size_t size = REG_SIZE_64;
    if (op->type == OPERAND_REG) size = op->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    /* F7 /2 - NOT r/m64 */
    if (emit_rex(ctx, is_64bit, NULL, op, NULL) < 0) return -1;
    if (emit_byte(ctx, 0xF7) < 0) return -1;
    if (emit_modrm_sib(ctx, 2, op, size) < 0) return -1;

    return 0;
}

/* SHL/SHR/SAL/SAR/ROL/ROR/RCL/RCR encoding */
int encode_shift(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count < 1 || inst->operand_count > 2) {
        fprintf(stderr, "Error: shift requires 1 or 2 operands\n");
        return -1;
    }

    const operand_t *op1 = &inst->operands[0];
    const operand_t *op2 = (inst->operand_count > 1) ? &inst->operands[1] : NULL;

    reg_size_t size = REG_SIZE_64;
    if (op1->type == OPERAND_REG) size = op1->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    uint8_t reg_opcode;
    switch (inst->type) {
        case INST_ROL: reg_opcode = 0; break;
        case INST_ROR: reg_opcode = 1; break;
        case INST_RCL: reg_opcode = 2; break;
        case INST_RCR: reg_opcode = 3; break;
        case INST_SHL:
        case INST_SAL: reg_opcode = 4; break;
        case INST_SHR: reg_opcode = 5; break;
        case INST_SAR: reg_opcode = 7; break;
        default:
            fprintf(stderr, "Error: Unknown shift instruction\n");
            return -1;
    }

    /* Determine count value */
    int count = 1;
    bool use_cl = false;

    if (op2) {
        if (op2->type == OPERAND_REG && op2->reg == 1 && op2->reg_size == REG_SIZE_8) {
            use_cl = true;
        } else if (op2->type == OPERAND_IMM) {
            count = (int)op2->immediate;
        } else {
            fprintf(stderr, "Error: shift count must be immediate or CL\n");
            return -1;
        }
    }

    uint8_t opcode = (size == REG_SIZE_8) ? 0xD0 : 0xD1;

    if (use_cl) {
        /* D2 /r or D3 /r - shift by CL */
        opcode = (size == REG_SIZE_8) ? 0xD2 : 0xD3;
        if (emit_rex(ctx, is_64bit, NULL, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_opcode, op1, size) < 0) return -1;
    } else if (count == 1) {
        /* D0 /r or D1 /r - shift by 1 */
        if (emit_rex(ctx, is_64bit, NULL, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_opcode, op1, size) < 0) return -1;
    } else {
        /* C0 /r ib or C1 /r ib - shift by immediate */
        opcode = (size == REG_SIZE_8) ? 0xC0 : 0xC1;
        if (emit_rex(ctx, is_64bit, NULL, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_opcode, op1, size) < 0) return -1;
        if (emit_byte(ctx, (uint8_t)count) < 0) return -1;
    }

    return 0;
}

/* BT/BTS/BTR/BTC encoding - Bit Test and Modify instructions */
int encode_bit_manip(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: %s requires 2 operands\n",
                inst->type == INST_BT ? "bt" :
                inst->type == INST_BTS ? "bts" :
                inst->type == INST_BTR ? "btr" : "btc");
        return -1;
    }

    const operand_t *op1 = &inst->operands[0];  /* Destination (register or memory) */
    const operand_t *op2 = &inst->operands[1];  /* Bit index (register or immediate) */

    /* Determine register size from destination */
    reg_size_t size = REG_SIZE_64;
    if (op1->type == OPERAND_REG) size = op1->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    /* Determine which sub-opcode to use based on instruction */
    uint8_t reg_opcode;
    switch (inst->type) {
        case INST_BT:
            reg_opcode = 4;  /* BT uses /4 in immediate form */
            break;
        case INST_BTS:
            reg_opcode = 5;  /* BTS uses /5 */
            break;
        case INST_BTR:
            reg_opcode = 6;  /* BTR uses /6 */
            break;
        case INST_BTC:
            reg_opcode = 7;  /* BTC uses /7 */
            break;
        default:
            fprintf(stderr, "Error: Unknown bit manipulation instruction\n");
            return -1;
    }

    /* Check if using immediate bit index */
    bool use_imm = (op2->type == OPERAND_IMM);
    bool use_cl = (op2->type == OPERAND_REG && op2->reg == 1); /* CL register */

    if (!use_imm && !use_cl) {
        fprintf(stderr, "Error: bit index must be immediate or CL register\n");
        return -1;
    }

    if (use_imm) {
        /* Immediate form: 0F BA /n ib */
        /* For BT with immediate, we always use /4 extension */
        uint8_t ext = (inst->type == INST_BT) ? 4 : reg_opcode;
        if (emit_rex(ctx, is_64bit, NULL, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x0F) < 0) return -1;
        if (emit_byte(ctx, 0xBA) < 0) return -1;
        if (emit_modrm_sib(ctx, ext, op1, size) < 0) return -1;
        if (emit_byte(ctx, (uint8_t)op2->immediate) < 0) return -1;
    } else {
        /* Register form: 0F A3/AB/B3/BB /r (BT/BTS/BTR/BTC) */
        uint8_t opcode2;
        switch (inst->type) {
            case INST_BT:  opcode2 = 0xA3; break;
            case INST_BTS: opcode2 = 0xAB; break;
            case INST_BTR: opcode2 = 0xB3; break;
            case INST_BTC: opcode2 = 0xBB; break;
            default: return -1;
        }

        /* For register form, we need to swap operand order in ModR/M:
         * The reg field contains the bit index register (CL)
         * The r/m field contains the destination */
        if (emit_rex(ctx, is_64bit, op2, op1, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x0F) < 0) return -1;
        if (emit_byte(ctx, opcode2) < 0) return -1;
        if (emit_modrm_sib(ctx, op2->reg, op1, size) < 0) return -1;
    }

    return 0;
}

/* SHLD/SHRD encoding - Double Precision Shift */
int encode_shld_shrd(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 3) {
        fprintf(stderr, "Error: %s requires 3 operands\n",
                inst->type == INST_SHLD ? "shld" : "shrd");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];   /* Destination register/memory */
    const operand_t *src = &inst->operands[1];   /* Source register */
    const operand_t *cnt = &inst->operands[2];   /* Count: immediate or CL */

    /* Both operands must be same size */
    reg_size_t size = REG_SIZE_64;
    if (dst->type == OPERAND_REG) size = dst->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    /* Source must be a register */
    if (src->type != OPERAND_REG) {
        fprintf(stderr, "Error: %s source operand must be a register\n",
                inst->type == INST_SHLD ? "shld" : "shrd");
        return -1;
    }

    /* Determine count type */
    bool use_cl = (cnt->type == OPERAND_REG && cnt->reg == 1);
    bool use_imm = (cnt->type == OPERAND_IMM);

    if (!use_cl && !use_imm) {
        fprintf(stderr, "Error: %s count must be immediate or CL\n",
                inst->type == INST_SHLD ? "shld" : "shrd");
        return -1;
    }

    uint8_t opcode2;
    if (inst->type == INST_SHLD) {
        opcode2 = use_cl ? 0xA5 : 0xA4;
    } else { /* INST_SHRD */
        opcode2 = use_cl ? 0xAD : 0xAC;
    }

    /* REX prefix - note: need REX.R for src register if r8-r15 */
    if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;
    if (emit_modrm_sib(ctx, src->reg, dst, size) < 0) return -1;

    /* Immediate byte if not using CL */
    if (use_imm) {
        if (emit_byte(ctx, (uint8_t)cnt->immediate) < 0) return -1;
    }

    return 0;
}

/* BSF/BSR encoding - Bit Scan Forward/Reverse */
int encode_bit_scan(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: %s requires 2 operands\n",
                inst->type == INST_BSF ? "bsf" : "bsr");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];  /* Destination register */
    const operand_t *src = &inst->operands[1];  /* Source (register or memory) */

    /* Destination must be a register */
    if (dst->type != OPERAND_REG) {
        fprintf(stderr, "Error: %s destination must be a register\n",
                inst->type == INST_BSF ? "bsf" : "bsr");
        return -1;
    }

    reg_size_t size = dst->reg_size;
    bool is_64bit = (size == REG_SIZE_64);

    uint8_t opcode2 = (inst->type == INST_BSF) ? 0xBC : 0xBD;

    /* REX prefix */
    if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;
    if (emit_modrm_sib(ctx, dst->reg, src, size) < 0) return -1;

    return 0;
}
