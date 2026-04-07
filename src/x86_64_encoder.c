/**
 * x86_64 Encoder - Core instruction encoding logic
 * Generates raw machine code bytes for x86_64 instructions
 */

#include "x86_64_asm/x86_64_asm.h"
#include "x86_64_asm/opcode_lookup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Standardized encoder diagnostic helpers for roadmap phase 2.1.1. */
static void encoder_diag_emit(int line, const char *category,
                              const char *message, const char *suggestion) {
    fprintf(stderr, "Error at line %d, column 1: [%s] %s\n", line, category, message);
    if (suggestion && suggestion[0] != '\0') {
        fprintf(stderr, "\nSuggestion: %s\n", suggestion);
    }
}

static void encoder_diagf(int line, const char *category,
                          const char *suggestion, const char *fmt, ...) {
    char message[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    encoder_diag_emit(line, category, message, suggestion);
}

/* ============================================================================
 * ENCODER-LOCAL VALIDATION HELPERS
 * ============================================================================ */

/* Check if an operand is a high 8-bit register (AH, BH, CH, DH). */
static int is_high_8bit_reg(const operand_t *op) {
    return op && op->type == OPERAND_REG && op->reg_size == REG_SIZE_8H;
}

/* Check for invalid combination: high 8-bit reg with any REX-requiring operand. */
static int check_rex_conflict(const parsed_instruction_t *inst, int op1_idx, int op2_idx) {
    const operand_t *op1 = (op1_idx >= 0 && op1_idx < inst->operand_count) ? &inst->operands[op1_idx] : NULL;
    const operand_t *op2 = (op2_idx >= 0 && op2_idx < inst->operand_count) ? &inst->operands[op2_idx] : NULL;

    int has_high_8bit = is_high_8bit_reg(op1) || is_high_8bit_reg(op2);
    int rex_required = (op1 && needs_rex(op1)) || (op2 && needs_rex(op2));
    return has_high_8bit && rex_required;
}

/* ============================================================================
 * INSTRUCTION ENCODING
 * ============================================================================ */

/* MOV instruction encoding */
int encode_mov(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "mov requires two operands: source and destination",
                      "mov requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Check for high 8-bit register with REX conflict */
    if (check_rex_conflict(inst, 0, 1)) {
        encoder_diagf(
            inst->line_number,
            "Constraint",
            "Use AL/BL/CL/DL or the low byte of R8-R15 instead.",
            "Cannot use AH/BH/CH/DH with registers R8-R15 or SPL/BPL/SIL/DIL"
        );
        return -1;
    }

    /* Determine operation size */
    reg_size_t size = REG_SIZE_64;
    if (dst->type == OPERAND_REG) size = dst->reg_size;
    else if (src->type == OPERAND_REG) size = src->reg_size;

    bool is_64bit = (size == REG_SIZE_64);
    bool need_rex_prefix = needs_rex(dst) || needs_rex(src);

    /* mov reg, label - Load value from data label (RIP-relative) */
    if (dst->type == OPERAND_REG && src->type == OPERAND_LABEL) {
        /* Treat as RIP-relative memory load: mov reg, [rip + label_offset] */
        /* This is MOV r64, r/m64 with ModRM using RIP-relative addressing */
        
        /* REX.W prefix for 64-bit */
        if (emit_rex(ctx, true, dst, NULL, NULL) < 0) return -1;
        
        /* MOV r64, r/m64 opcode: 8B /r */
        if (emit_byte(ctx, 0x8B) < 0) return -1;
        
        /* ModRM: mod=00 (RIP-relative), reg=dst, r/m=101 (RIP-relative indicator) */
        uint8_t modrm = make_modrm(0, dst->reg & 7, 5);
        if (emit_byte(ctx, modrm) < 0) return -1;
        
        /* Add RIP-relative fixup and emit placeholder displacement */
        if (add_fixup_rip_relative(ctx, src->label, ctx->current_address, ctx->current_section) < 0) {
            return -1;
        }
        if (emit_le32(ctx, 0) < 0) return -1;
        return 0;
    }

    /* mov reg, imm - MOV r64, imm64 or MOV r32, imm32 or MOV r16, imm16 or MOV r8, imm8 */
    if (dst->type == OPERAND_REG && src->type == OPERAND_IMM) {
        int reg_val = dst->reg & 7;
        bool fits_u32 = (src->immediate >= 0 && src->immediate <= UINT32_MAX);

        if (is_64bit && fits_u32) {
            /* Canonical form for many assemblers: B8+rd imm32 (zero-extends into r64). */
            if (dst->reg >= 8) {
                if (emit_byte(ctx, make_rex(false, false, false, true)) < 0) return -1;
            }
            if (emit_byte(ctx, (uint8_t)(0xB8 + reg_val)) < 0) return -1;
            if (emit_le32(ctx, (int32_t)(uint32_t)src->immediate) < 0) return -1;
        } else if (is_64bit && (src->immediate < INT32_MIN || src->immediate > INT32_MAX)) {
            /* MOV r64, imm64 - special encoding: REX.W + C7 /0 followed by imm64,
             * but actually the shorter form for certain regs uses B8+rq */
            uint8_t opcode = (uint8_t)(0xB8 + reg_val);
            if (emit_rex(ctx, true, dst, NULL, NULL) < 0) return -1;
            if (emit_byte(ctx, opcode) < 0) return -1;
            if (emit_le64(ctx, src->immediate) < 0) return -1;
        } else if (is_64bit) {
            /* MOV r/m64, imm32 - sign-extended */
            uint8_t opcode = 0xC7;
            if (emit_rex(ctx, true, NULL, dst, NULL) < 0) return -1;
            if (emit_byte(ctx, opcode) < 0) return -1;
            if (emit_modrm_sib(ctx, 0, dst, size) < 0) return -1;
            if (emit_le32(ctx, (int32_t)src->immediate) < 0) return -1;
        } else if (size == REG_SIZE_32) {
            /* MOV r32, imm32 */
            if (need_rex_prefix) {
                if (emit_rex(ctx, false, dst, NULL, NULL) < 0) return -1;
            }
            if (emit_byte(ctx, (uint8_t)(0xB8 + reg_val)) < 0) return -1;
            if (emit_le32(ctx, (int32_t)src->immediate) < 0) return -1;
        } else if (size == REG_SIZE_16) {
            /* MOV r16, imm16 - need 0x66 prefix */
            if (emit_byte(ctx, 0x66) < 0) return -1;
            if (need_rex_prefix) {
                if (emit_rex(ctx, false, dst, NULL, NULL) < 0) return -1;
            }
            if (emit_byte(ctx, (uint8_t)(0xB8 + reg_val)) < 0) return -1;
            if (emit_le16(ctx, (int16_t)src->immediate) < 0) return -1;
        } else {
            /* MOV r8, imm8 */
            if (dst->reg_size == REG_SIZE_8H) {
                /* AH, BH, CH, DH - no REX allowed */
                if (emit_byte(ctx, (uint8_t)(0xB0 + reg_val)) < 0) return -1;
            } else {
                if (emit_rex(ctx, false, dst, NULL, NULL) < 0) return -1;
                if (emit_byte(ctx, (uint8_t)(0xB0 + reg_val)) < 0) return -1;
            }
            if (emit_byte(ctx, (uint8_t)src->immediate) < 0) return -1;
        }
        return 0;
    }

    /* mov reg, reg - MOV r/m64, r64 or MOV r64, r/m64 */
    if (dst->type == OPERAND_REG && src->type == OPERAND_REG) {
        /* MOV r/m64, r64 (store) */
        if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x89) < 0) return -1;
        if (emit_modrm_sib(ctx, src->reg & 7, dst, size) < 0) return -1;
        return 0;
    }

    /* mov reg, mem - MOV r64, r/m64 */
    if (dst->type == OPERAND_REG && src->type == OPERAND_MEM) {
        if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x8B) < 0) return -1;
        if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
        return 0;
    }

    /* mov mem, reg - MOV r/m64, r64 */
    if (dst->type == OPERAND_MEM && src->type == OPERAND_REG) {
        if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
        if (emit_byte(ctx, 0x89) < 0) return -1;
        if (emit_modrm_sib(ctx, src->reg & 7, dst, size) < 0) return -1;
        return 0;
    }

    /* mov mem, imm - MOV r/m64, imm32 (sign-extended) or MOV r/m32, imm32 etc */
    if (dst->type == OPERAND_MEM && src->type == OPERAND_IMM) {
        if (size == REG_SIZE_16) {
            if (emit_byte(ctx, 0x66) < 0) return -1;
        }
        if (emit_rex(ctx, is_64bit, NULL, dst, NULL) < 0) return -1;

        if (size == REG_SIZE_8) {
            if (emit_byte(ctx, 0xC6) < 0) return -1;
        } else {
            if (emit_byte(ctx, 0xC7) < 0) return -1;
        }

        if (emit_modrm_sib(ctx, 0, dst, size) < 0) return -1;

        if (size == REG_SIZE_8) {
            if (emit_byte(ctx, (uint8_t)src->immediate) < 0) return -1;
        } else if (size == REG_SIZE_16) {
            if (emit_le16(ctx, (int16_t)src->immediate) < 0) return -1;
        } else {
            if (emit_le32(ctx, (int32_t)src->immediate) < 0) return -1;
        }
        return 0;
    }

    encoder_diagf(inst->line_number, "Encoding",
                  "Supported forms: mov reg, reg | mov reg, imm | mov reg, mem | mov mem, reg | mov mem, imm",
                  "Unsupported mov operand combination");
    return -1;
}

/* PUSH/POP encoding */
int encode_push_pop(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        encoder_diagf(inst->line_number, "Encoding",
                      inst->type == INST_PUSH ? "push requires one operand: register, memory, or immediate"
                                              : "pop requires one operand: register or memory",
                      "%s requires 1 operand", inst->type == INST_PUSH ? "push" : "pop");
        return -1;
    }

    const operand_t *op = &inst->operands[0];
    bool is_push = (inst->type == INST_PUSH);

    if (op->type == OPERAND_REG) {
        int reg = op->reg & 7;

        if (op->reg >= 8) {
            /* REX.B prefix for R8-R15 */
            uint8_t rex = make_rex(false, false, false, true);
            if (emit_byte(ctx, rex) < 0) return -1;
        }

        /* 50+rq for PUSH, 58+rq for POP */
        uint8_t opcode = (uint8_t)(is_push ? (0x50 + reg) : (0x58 + reg));
        if (emit_byte(ctx, opcode) < 0) return -1;
        return 0;
    }

    if (op->type == OPERAND_IMM) {
        if (!is_push) {
            encoder_diagf(inst->line_number, "Encoding",
                          "pop only accepts register or memory operands, not immediates",
                          "pop with immediate not valid");
            return -1;
        }

        if (fits_in_int8((int32_t)op->immediate)) {
            /* PUSH imm8 */
            if (emit_byte(ctx, 0x6A) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)(int8_t)op->immediate) < 0) return -1;
        } else {
            /* PUSH imm32 (sign-extended to 64-bit) */
            if (emit_byte(ctx, 0x68) < 0) return -1;
            if (emit_le32(ctx, (int32_t)op->immediate) < 0) return -1;
        }
        return 0;
    }

    if (op->type == OPERAND_MEM) {
        /* FF /6 for PUSH, 8F /0 for POP */
        if (emit_rex(ctx, true, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, is_push ? 0xFF : 0x8F) < 0) return -1;
        if (emit_modrm_sib(ctx, is_push ? 6 : 0, op, REG_SIZE_64) < 0) return -1;
        return 0;
    }

    encoder_diagf(inst->line_number, "Encoding",
                  "Supported forms: push reg | push mem | push imm | pop reg | pop mem",
                  "Unsupported push/pop operand");
    return -1;
}

/* Arithmetic instructions (ADD, SUB, AND, OR, XOR, CMP) */
int encode_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Arithmetic instructions require two operands: destination and source",
                      "Arithmetic instruction requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Check for high 8-bit register with REX conflict */
    if (check_rex_conflict(inst, 0, 1)) {
        encoder_diagf(
            inst->line_number,
            "Constraint",
            "Use AL/BL/CL/DL or the low byte of R8-R15 instead.",
            "Cannot use AH/BH/CH/DH with registers R8-R15 or SPL/BPL/SIL/DIL"
        );
        return -1;
    }

    /* Determine operation size */
    reg_size_t size = REG_SIZE_64;
    if (dst->type == OPERAND_REG) size = dst->reg_size;
    else if (src->type == OPERAND_REG) size = src->reg_size;

    bool is_64bit = (size == REG_SIZE_64);

    /* Get the reg/opcode field for ModR/M */
    uint8_t reg_opcode;
    if (x86_lookup_group1_reg_opcode(inst->type, &reg_opcode) < 0) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Supported: add, sub, and, or, xor, cmp, adc, sbb",
                      "Unknown arithmetic instruction");
        return -1;
    }

    /* ADD/SUB/AND/OR/XOR/CMP reg, imm */
    if (dst->type == OPERAND_REG && src->type == OPERAND_IMM) {
        int64_t imm64 = src->immediate;
        int32_t imm32 = (int32_t)imm64;
        bool fits8 = fits_in_int8(imm32);
        
        /* For 64-bit operations, check if immediate fits in signed 32-bit.
         * If not, we need to use a different encoding (load to register first).
         * The imm32 forms sign-extend to 64 bits, so they can only represent
         * values from -2^31 to 2^31-1. */
        bool fits_signed32 = (imm64 >= INT32_MIN && imm64 <= INT32_MAX);

        /* Special case: RAX/AL with imm - shorter encoding.
         * Only use if the value fits in signed 32-bit for 64-bit operations. */
        if (dst->reg == 0 && !fits8 && (size == REG_SIZE_64 || size == REG_SIZE_32) &&
            (size != REG_SIZE_64 || fits_signed32)) {
            /* 05 id (ADD), 0D id (OR), 15 id (ADC), 1D id (SBB),
               25 id (AND), 2D id (SUB), 35 id (XOR), 3D id (CMP) */
            uint8_t opcode = (uint8_t)(0x05 + (reg_opcode << 3));
            if (is_64bit && emit_rex(ctx, true, dst, NULL, NULL) < 0) return -1;
            if (emit_byte(ctx, opcode) < 0) return -1;
            if (emit_le32(ctx, imm32) < 0) return -1;
            return 0;
        }

        /* For 64-bit operations, if value doesn't fit in signed 32-bit,
         * we need to load it into a register first. This is because all
         * immediate forms sign-extend to 64 bits. */
        if (is_64bit && !fits_signed32) {
            encoder_diagf(inst->line_number, "Constraint",
                          "Load the 64-bit immediate into a register first using 'mov reg, imm64'",
                          "64-bit immediate 0x%llx out of range for %s, use mov reg, imm64 first",
                          (unsigned long long)imm64,
                          inst->type == INST_CMP ? "cmp" :
                          inst->type == INST_ADD ? "add" :
                          inst->type == INST_SUB ? "sub" : "op");
            return -1;
        }

        /* Use imm8 form if possible */
        if (fits8 && size != REG_SIZE_8) {
            /* 83 /r ib - sign-extended 8-bit immediate */
            if (is_64bit && emit_rex(ctx, true, NULL, dst, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x83) < 0) return -1;
            if (emit_modrm_sib(ctx, reg_opcode, dst, size) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)imm32) < 0) return -1;
        } else {
            /* 81 /r id - 32-bit immediate */
            if (is_64bit && emit_rex(ctx, true, NULL, dst, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x81) < 0) return -1;
            if (emit_modrm_sib(ctx, reg_opcode, dst, size) < 0) return -1;
            if (emit_le32(ctx, imm32) < 0) return -1;
        }
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP reg, reg */
    if (dst->type == OPERAND_REG && src->type == OPERAND_REG) {
        /* dst = dst op src: 01 /r (ADD), 09 /r (OR), 11 /r (ADC), 19 /r (SBB),
                             21 /r (AND), 29 /r (SUB), 31 /r (XOR), 39 /r (CMP) */
        uint8_t opcode = (uint8_t)(0x01 + (reg_opcode << 3));
        if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, src->reg & 7, dst, size) < 0) return -1;
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP reg, mem */
    if (dst->type == OPERAND_REG && src->type == OPERAND_MEM) {
        uint8_t opcode = (uint8_t)(0x03 + (reg_opcode << 3)); /* 03 /r (ADD), 0B /r (OR), etc */
        if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP mem, reg */
    if (dst->type == OPERAND_MEM && src->type == OPERAND_REG) {
        uint8_t opcode = (uint8_t)(0x01 + (reg_opcode << 3));
        if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, src->reg & 7, dst, size) < 0) return -1;
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP mem, imm */
    if (dst->type == OPERAND_MEM && src->type == OPERAND_IMM) {
        int32_t imm = (int32_t)src->immediate;
        bool fits8 = fits_in_int8(imm);

        if (fits8) {
            if (emit_rex(ctx, is_64bit, NULL, dst, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x83) < 0) return -1;
            if (emit_modrm_sib(ctx, reg_opcode, dst, size) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)imm) < 0) return -1;
        } else {
            if (emit_rex(ctx, is_64bit, NULL, dst, NULL) < 0) return -1;
            if (emit_byte(ctx, 0x81) < 0) return -1;
            if (emit_modrm_sib(ctx, reg_opcode, dst, size) < 0) return -1;
            if (emit_le32(ctx, imm) < 0) return -1;
        }
        return 0;
    }

    encoder_diagf(inst->line_number, "Encoding",
                  "Supported forms: op reg, imm | op reg, reg | op reg, mem | op mem, reg | op mem, imm",
                  "Unsupported arithmetic operand combination");
    return -1;
}

/* INC/DEC/NEG/NOT encoding */
int encode_unary_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        encoder_diagf(inst->line_number, "Encoding",
                      "inc, dec, neg, and not require one operand: register or memory",
                      "Unary instruction requires 1 operand");
        return -1;
    }

    const operand_t *op = &inst->operands[0];

    reg_size_t size = REG_SIZE_64;
    if (op->type == OPERAND_REG) size = op->reg_size;

    bool is_64bit = (size == REG_SIZE_64);

    uint8_t reg_opcode;
    switch (inst->type) {
        case INST_INC: reg_opcode = 0; break;
        case INST_DEC: reg_opcode = 1; break;
        case INST_NOT: reg_opcode = 2; break;
        case INST_NEG: reg_opcode = 3; break;
        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: inc, dec, not, neg",
                          "Unknown unary instruction");
            return -1;
    }

    /* FF /r for INC/DEC, F7 /r for NOT/NEG */
    uint8_t opcode = (inst->type == INST_INC || inst->type == INST_DEC) ? 0xFF : 0xF7;

    if (op->type == OPERAND_REG) {
        if (emit_rex(ctx, is_64bit, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_opcode, op, size) < 0) return -1;
        return 0;
    }

    if (op->type == OPERAND_MEM) {
        if (emit_rex(ctx, is_64bit, NULL, op, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, reg_opcode, op, size) < 0) return -1;
        return 0;
    }

    encoder_diagf(inst->line_number, "Encoding",
                  "Supported forms: inc reg | inc mem | dec reg | dec mem | not reg | not mem | neg reg | neg mem",
                  "Unsupported unary operand");
    return -1;
}

/* LEA encoding */
int encode_lea(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "lea requires two operands: destination register and memory source",
                      "lea requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    if (dst->type != OPERAND_REG || src->type != OPERAND_MEM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "lea requires a register destination and a memory source (e.g., lea rax, [rbx + 8])",
                      "lea requires register destination and memory source");
        return -1;
    }

    reg_size_t size = dst->reg_size;
    if (size == REG_SIZE_8 || size == REG_SIZE_8H) {
        encoder_diagf(inst->line_number, "Encoding",
                      "lea destination must be 16, 32, or 64-bit register (ax, eax, rax)",
                      "lea destination must be 16, 32, or 64-bit register");
        return -1;
    }

    if (size == REG_SIZE_16) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }

    bool is_64bit = (size == REG_SIZE_64);

    /* 8D /r - LEA r64, m */
    if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
    if (emit_byte(ctx, 0x8D) < 0) return -1;
    if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;

    return 0;
}

/* String instruction encoding (movsb, movsw, movsd, movsq) */
int encode_string(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 0) {
        encoder_diagf(inst->line_number, "Encoding",
                      "String instructions (movsb, movsw, movsd, movsq) take no operands",
                      "String instruction takes no operands");
        return -1;
    }

    switch (inst->type) {
        case INST_MOVSB:
            /* MOVS/B: A4 */
            if (emit_byte(ctx, 0xA4) < 0) return -1;
            break;
        case INST_MOVSW:
            /* MOVS/W: 66 A5 */
            if (emit_byte(ctx, 0x66) < 0) return -1;
            if (emit_byte(ctx, 0xA5) < 0) return -1;
            break;
        case INST_MOVSD:
            /* MOVS/D: A5 */
            if (emit_byte(ctx, 0xA5) < 0) return -1;
            break;
        case INST_MOVSQ:
            /* MOVS/Q: 48 A5 (REX.W + A5) */
            if (emit_byte(ctx, 0x48) < 0) return -1;
            if (emit_byte(ctx, 0xA5) < 0) return -1;
            break;
        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: movsb, movsw, movsd, movsq",
                          "Unknown string instruction");
            return -1;
    }

    return 0;
}

/* ============================================================================
 * SSE INSTRUCTION ENCODING
 * ============================================================================ */

/* Check if a register is an XMM register */
static int is_xmm_reg(reg_t reg) {
    return reg >= XMM0 && reg <= XMM15;
}

/* SSE Move Instruction Encoding
 * Supports: MOVAPS, MOVUPS, MOVSS, MOVAPD, MOVUPD, MOVSD
 */
int encode_sse_mov(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use exactly two operands, for example: movups xmm0, [rax] or movups [rax], xmm0.",
            "SSE move requires 2 operands"
        );
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Determine operand types */
    bool dst_is_xmm = (dst->type == OPERAND_REG && is_xmm_reg(dst->reg));
    bool src_is_xmm = (src->type == OPERAND_REG && is_xmm_reg(src->reg));
    bool src_is_mem = (src->type == OPERAND_MEM);
    bool dst_is_mem = (dst->type == OPERAND_MEM);

    /* Validate XMM register operands use 128-bit register size metadata. */
    if (dst_is_xmm && dst->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE move XMM destination must use xmm register size");
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE move XMM source must use xmm register size");
        return -1;
    }

    /* Validate memory form first for clearer diagnostics. */
    if (dst_is_mem && src_is_mem) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use one memory operand and one XMM register operand.",
            "SSE move cannot have two memory operands"
        );
        return -1;
    }

    /* Form validation:
     * - dst=xmm allows src=xmm|mem
     * - dst=mem allows src=xmm only
     */
    if (dst_is_mem && !src_is_xmm) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use an XMM source register when writing to memory, e.g. movups [rax], xmm0.",
            "SSE move memory destination requires XMM source"
        );
        return -1;
    }
    if (dst_is_xmm && !src_is_xmm && !src_is_mem) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use an XMM register or memory operand as the source for an XMM destination.",
            "SSE move XMM destination requires XMM or memory source"
        );
        return -1;
    }

    /* Validate: at least one XMM operand required for SSE move. */
    if (!dst_is_xmm && !src_is_xmm) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use at least one XMM register operand for SSE moves.",
            "SSE move requires at least one XMM operand"
        );
        return -1;
    }

    /* Determine prefixes and opcode */
    bool need_66_prefix = false;  /* 0x66 for double-precision */
    bool need_f3_prefix = false;  /* 0xF3 for scalar single */
    bool need_f2_prefix = false;  /* 0xF2 for scalar double */
    uint8_t opcode2;              /* Second byte after 0x0F */

    switch (inst->type) {
        case INST_MOVAPS:
            /* 0F 28 /r - MOVAPS xmm1, xmm2/m128 (load) */
            /* 0F 29 /r - MOVAPS xmm2/m128, xmm1 (store) */
            opcode2 = dst_is_mem ? 0x29 : 0x28;
            break;

        case INST_MOVUPS:
            /* 0F 10 /r - MOVUPS xmm1, xmm2/m128 (load) */
            /* 0F 11 /r - MOVUPS xmm2/m128, xmm1 (store) */
            opcode2 = dst_is_mem ? 0x11 : 0x10;
            break;

        case INST_MOVSS:
            /* F3 0F 10 /r - MOVSS xmm1, xmm2/m32 (load) */
            /* F3 0F 11 /r - MOVSS xmm2/m32, xmm1 (store) */
            need_f3_prefix = true;
            opcode2 = dst_is_mem ? 0x11 : 0x10;
            break;

        case INST_MOVAPD:
            /* 66 0F 28 /r - MOVAPD xmm1, xmm2/m128 (load) */
            /* 66 0F 29 /r - MOVAPD xmm2/m128, xmm1 (store) */
            need_66_prefix = true;
            opcode2 = dst_is_mem ? 0x29 : 0x28;
            break;

        case INST_MOVUPD:
            /* 66 0F 10 /r - MOVUPD xmm1, xmm2/m128 (load) */
            /* 66 0F 11 /r - MOVUPD xmm2/m128, xmm1 (store) */
            need_66_prefix = true;
            opcode2 = dst_is_mem ? 0x11 : 0x10;
            break;

        case INST_MOVSD_XMM:
            /* F2 0F 10 /r - MOVSD xmm1, xmm2/m64 (load) */
            /* F2 0F 11 /r - MOVSD xmm2/m64, xmm1 (store) */
            need_f2_prefix = true;
            opcode2 = dst_is_mem ? 0x11 : 0x10;
            break;

        case INST_MOVDQA:
            /* 66 0F 6F /r - MOVDQA xmm1, xmm2/m128 (load aligned) */
            /* 66 0F 7F /r - MOVDQA xmm2/m128, xmm1 (store aligned) */
            need_66_prefix = true;
            opcode2 = dst_is_mem ? 0x7F : 0x6F;
            break;

        case INST_MOVDQU:
            /* F3 0F 6F /r - MOVDQU xmm1, xmm2/m128 (load unaligned) */
            /* F3 0F 7F /r - MOVDQU xmm2/m128, xmm1 (store unaligned) */
            need_f3_prefix = true;
            opcode2 = dst_is_mem ? 0x7F : 0x6F;
            break;

        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: movaps, movups, movss, movapd, movupd, movsd, movdqa, movdqu",
                          "Unknown SSE move instruction");
            return -1;
    }

    /* Determine REX prefix requirements */
    /* XMM8-XMM15 use REX.R (for reg field) and REX.B (for r/m field) */
    bool rex_r = false;
    bool rex_b = false;
    bool rex_x = false;

    /* For register operands in ModR/M reg field (destination in reg-to-reg, source in store) */
    const operand_t *reg_field_op = dst_is_mem ? src : dst;
    if (reg_field_op->type == OPERAND_REG && reg_field_op->reg >= XMM8) {
        rex_r = true;
    }

    /* For register operands in ModR/M r/m field (source in reg-to-reg, destination in store-to-reg) */
    const operand_t *rm_field_op = dst_is_mem ? dst : src;
    if (rm_field_op->type == OPERAND_REG && rm_field_op->reg >= XMM8) {
        rex_b = true;
    }

    /* For memory operands with extended index */
    if (src_is_mem && src->mem.has_index && src->mem.index >= 8) {
        rex_x = true;
    }
    if (dst_is_mem && dst->mem.has_index && dst->mem.index >= 8) {
        rex_x = true;
    }
    /* For memory operands with extended base */
    if (src_is_mem && src->mem.has_base && src->mem.base >= 8) {
        rex_b = true;
    }
    if (dst_is_mem && dst->mem.has_base && dst->mem.base >= 8) {
        rex_b = true;
    }

    /* Emit instruction prefixes (order matters: 66/F2/F3 before REX) */
    if (need_66_prefix) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }
    if (need_f2_prefix) {
        if (emit_byte(ctx, 0xF2) < 0) return -1;
    }
    if (need_f3_prefix) {
        if (emit_byte(ctx, 0xF3) < 0) return -1;
    }

    /* Emit REX prefix if needed (REX.W is 0 for SSE - no 64-bit extension) */
    if (rex_r || rex_x || rex_b) {
        if (emit_byte(ctx, make_rex(false, rex_r, rex_x, rex_b)) < 0) return -1;
    }

    /* Emit two-byte opcode escape and second byte */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;

    /* Emit ModR/M byte */
    if (dst_is_mem) {
        /* Store: src XMM is reg field, dst memory is r/m field */
        if (emit_modrm_sib(ctx, src->reg & 7, dst, REG_SIZE_XMM) < 0) return -1;
    } else if (src_is_mem) {
        /* Load: dst XMM is reg field, src memory is r/m field */
        if (emit_modrm_sib(ctx, dst->reg & 7, src, REG_SIZE_XMM) < 0) return -1;
    } else {
        /* Register to register: mod=11, reg=dst, r/m=src */
        uint8_t modrm = make_modrm(3, dst->reg & 7, src->reg & 7);
        if (emit_byte(ctx, modrm) < 0) return -1;
    }

    return 0;
}

/* SSE Arithmetic Instruction Encoding
 * Supports: ADDSS/PS/SD/PD, SUBSS/PS/SD/PD, MULSS/PS/SD/PD, DIVSS/PS/SD/PD
 */
int encode_sse_arith(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use exactly two operands, e.g. addps xmm0, xmm1 or addps xmm0, [rax].",
            "SSE arithmetic requires 2 operands"
        );
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Validate destination is XMM register */
    if (dst->type != OPERAND_REG || !is_xmm_reg(dst->reg)) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use an XMM destination register, e.g. addps xmm0, xmm1.",
            "SSE arithmetic destination must be XMM register"
        );
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE arithmetic destination must use xmm register size");
        return -1;
    }

    /* Validate source is XMM register or memory */
    bool src_is_xmm = (src->type == OPERAND_REG && is_xmm_reg(src->reg));
    bool src_is_mem = (src->type == OPERAND_MEM);

    if (!src_is_xmm && !src_is_mem) {
        encoder_diagf(
            inst->line_number,
            "Encoding",
            "Use an XMM register or memory source operand.",
            "SSE arithmetic source must be XMM or memory"
        );
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE arithmetic XMM source must use xmm register size");
        return -1;
    }

    /* Determine prefixes and opcode */
    bool need_66_prefix = false;
    bool need_f3_prefix = false;
    bool need_f2_prefix = false;
    uint8_t opcode2;

    switch (inst->type) {
        /* ADD operations */
        case INST_ADDSS: need_f3_prefix = true; opcode2 = 0x58; break;
        case INST_ADDPS:                          opcode2 = 0x58; break;
        case INST_ADDSD: need_f2_prefix = true; opcode2 = 0x58; break;
        case INST_ADDPD: need_66_prefix = true; opcode2 = 0x58; break;

        /* SUB operations */
        case INST_SUBSS: need_f3_prefix = true; opcode2 = 0x5C; break;
        case INST_SUBPS:                          opcode2 = 0x5C; break;
        case INST_SUBSD: need_f2_prefix = true; opcode2 = 0x5C; break;
        case INST_SUBPD: need_66_prefix = true; opcode2 = 0x5C; break;

        /* MUL operations */
        case INST_MULSS: need_f3_prefix = true; opcode2 = 0x59; break;
        case INST_MULPS:                          opcode2 = 0x59; break;
        case INST_MULSD: need_f2_prefix = true; opcode2 = 0x59; break;
        case INST_MULPD: need_66_prefix = true; opcode2 = 0x59; break;

        /* DIV operations */
        case INST_DIVSS: need_f3_prefix = true; opcode2 = 0x5E; break;
        case INST_DIVPS:                          opcode2 = 0x5E; break;
        case INST_DIVSD: need_f2_prefix = true; opcode2 = 0x5E; break;
        case INST_DIVPD: need_66_prefix = true; opcode2 = 0x5E; break;

        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: addss, addps, addsd, addpd, subss, subps, subsd, subpd, mulss, mulps, mulsd, mulpd, divss, divps, divsd, divpd",
                          "Unknown SSE arithmetic instruction");
            return -1;
    }

    /* Determine REX prefix requirements */
    bool rex_r = (dst->reg >= XMM8);
    bool rex_b = false;
    bool rex_x = false;

    if (src_is_xmm && src->reg >= XMM8) {
        rex_b = true;
    } else if (src_is_mem) {
        if (src->mem.has_base && src->mem.base >= 8) rex_b = true;
        if (src->mem.has_index && src->mem.index >= 8) rex_x = true;
    }

    /* Emit instruction prefixes */
    if (need_66_prefix) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }
    if (need_f2_prefix) {
        if (emit_byte(ctx, 0xF2) < 0) return -1;
    }
    if (need_f3_prefix) {
        if (emit_byte(ctx, 0xF3) < 0) return -1;
    }

    /* Emit REX prefix if needed */
    if (rex_r || rex_x || rex_b) {
        if (emit_byte(ctx, make_rex(false, rex_r, rex_x, rex_b)) < 0) return -1;
    }

    /* Emit two-byte opcode escape and second byte */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;

    /* Emit ModR/M: dst XMM is always reg field, src is r/m field */
    if (emit_modrm_sib(ctx, dst->reg & 7, src, REG_SIZE_XMM) < 0) return -1;

    return 0;
}

/* SSE Packed Integer Arithmetic Instruction Encoding
 * Supports: PADDB/PADDW/PADDD/PADDQ, PSUBB/PSUBW/PSUBD/PSUBQ
 */
int encode_sse_packed_arith(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use exactly two operands: destination XMM register and source (XMM or memory)",
                      "SSE packed arithmetic requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Determine operand types */
    bool dst_is_xmm = (dst->type == OPERAND_REG && is_xmm_reg(dst->reg));
    bool src_is_xmm = (src->type == OPERAND_REG && is_xmm_reg(src->reg));
    bool src_is_mem = (src->type == OPERAND_MEM);

    /* Validate XMM register operands use 128-bit register size metadata */
    if (!dst_is_xmm) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use an XMM destination register, e.g. paddb xmm0, xmm1.",
                      "SSE packed arithmetic destination must be XMM register");
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE packed arithmetic XMM destination must use xmm register size");
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE packed arithmetic XMM source must use xmm register size");
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Source must be an XMM register or memory operand",
                      "SSE packed arithmetic source must be XMM or memory");
        return -1;
    }

    /* Determine prefixes and opcode */
    bool need_66_prefix = true;  /* All packed integer ops need 0x66 prefix */
    uint8_t opcode2;              /* Second byte after 0x0F */

    switch (inst->type) {
        case INST_PADDB:
            opcode2 = 0xFC;  /* 66 0F FC /r - PADDB xmm1, xmm2/m128 */
            break;
        case INST_PADDW:
            opcode2 = 0xFD;  /* 66 0F FD /r - PADDW xmm1, xmm2/m128 */
            break;
        case INST_PADDD:
            opcode2 = 0xFE;  /* 66 0F FE /r - PADDD xmm1, xmm2/m128 */
            break;
        case INST_PADDQ:
            opcode2 = 0xD4;  /* 66 0F D4 /r - PADDQ xmm1, xmm2/m128 */
            break;
        case INST_PSUBB:
            opcode2 = 0xF8;  /* 66 0F F8 /r - PSUBB xmm1, xmm2/m128 */
            break;
        case INST_PSUBW:
            opcode2 = 0xF9;  /* 66 0F F9 /r - PSUBW xmm1, xmm2/m128 */
            break;
        case INST_PSUBD:
            opcode2 = 0xFA;  /* 66 0F FA /r - PSUBD xmm1, xmm2/m128 */
            break;
        case INST_PSUBQ:
            opcode2 = 0xFB;  /* 66 0F FB /r - PSUBQ xmm1, xmm2/m128 */
            break;
        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: paddb, paddw, paddd, paddq, psubb, psubw, psubd, psubq",
                          "Unknown SSE packed arithmetic instruction");
            return -1;
    }

    /* Determine REX prefix requirements */
    bool rex_r = (dst->reg >= XMM8);
    bool rex_b = false;
    bool rex_x = false;

    if (src_is_xmm && src->reg >= XMM8) {
        rex_b = true;
    } else if (src_is_mem) {
        if (src->mem.has_base && src->mem.base >= 8) rex_b = true;
        if (src->mem.has_index && src->mem.index >= 8) rex_x = true;
    }

    /* Emit instruction prefixes */
    if (need_66_prefix) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }

    /* Emit REX prefix if needed */
    if (rex_r || rex_x || rex_b) {
        if (emit_byte(ctx, make_rex(false, rex_r, rex_x, rex_b)) < 0) return -1;
    }

    /* Emit two-byte opcode escape and second byte */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;

    /* Emit ModR/M: dst XMM is always reg field, src is r/m field */
    if (emit_modrm_sib(ctx, dst->reg & 7, src, REG_SIZE_XMM) < 0) return -1;

    return 0;
}

/* SSE Packed Compare Instruction Encoding
 * Supports: PCMPEQB/PCMPEQW/PCMPEQD/PCMPEQQ, PCMPGTB/PCMPGTW/PCMPGTD/PCMPGTQ
 */
int encode_sse_packed_cmp(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use exactly two operands: destination XMM register and source (XMM or memory)",
                      "SSE packed compare requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Determine operand types */
    bool dst_is_xmm = (dst->type == OPERAND_REG && is_xmm_reg(dst->reg));
    bool src_is_xmm = (src->type == OPERAND_REG && is_xmm_reg(src->reg));
    bool src_is_mem = (src->type == OPERAND_MEM);

    /* Validate XMM register operands use 128-bit register size metadata */
    if (!dst_is_xmm) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use an XMM destination register, e.g. pcmpeqb xmm0, xmm1.",
                      "SSE packed compare destination must be XMM register");
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE packed compare XMM destination must use xmm register size");
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE packed compare XMM source must use xmm register size");
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Source must be an XMM register or memory operand",
                      "SSE packed compare source must be XMM or memory");
        return -1;
    }

    /* Determine opcode */
    bool need_66_prefix = true;   /* All packed compare ops need 0x66 prefix */
    bool need_0f38_escape = false;
    uint8_t opcode2;              /* Final opcode byte */

    switch (inst->type) {
        case INST_PCMPEQB:
            opcode2 = 0x74;  /* 66 0F 74 /r - PCMPEQB xmm1, xmm2/m128 */
            break;
        case INST_PCMPEQW:
            opcode2 = 0x75;  /* 66 0F 75 /r - PCMPEQW xmm1, xmm2/m128 */
            break;
        case INST_PCMPEQD:
            opcode2 = 0x76;  /* 66 0F 76 /r - PCMPEQD xmm1, xmm2/m128 */
            break;
        case INST_PCMPEQQ:
            opcode2 = 0x29;  /* 66 0F 38 29 /r - PCMPEQQ xmm1, xmm2/m128 (3-byte opcode) */
            need_0f38_escape = true;
            break;
        case INST_PCMPGTB:
            opcode2 = 0x64;  /* 66 0F 64 /r - PCMPGTB xmm1, xmm2/m128 */
            break;
        case INST_PCMPGTW:
            opcode2 = 0x65;  /* 66 0F 65 /r - PCMPGTW xmm1, xmm2/m128 */
            break;
        case INST_PCMPGTD:
            opcode2 = 0x66;  /* 66 0F 66 /r - PCMPGTD xmm1, xmm2/m128 */
            break;
        case INST_PCMPGTQ:
            opcode2 = 0x37;  /* 66 0F 38 37 /r - PCMPGTQ xmm1, xmm2/m128 (3-byte opcode) */
            need_0f38_escape = true;
            break;
        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: pcmpeqb, pcmpeqw, pcmpeqd, pcmpeqq, pcmpgtb, pcmpgtw, pcmpgtd, pcmpgtq",
                          "Unknown SSE packed compare instruction");
            return -1;
    }

    /* Determine REX prefix requirements */
    bool rex_r = (dst->reg >= XMM8);
    bool rex_b = false;
    bool rex_x = false;

    if (src_is_xmm && src->reg >= XMM8) {
        rex_b = true;
    } else if (src_is_mem) {
        if (src->mem.has_base && src->mem.base >= 8) rex_b = true;
        if (src->mem.has_index && src->mem.index >= 8) rex_x = true;
    }

    /* Emit instruction prefixes */
    if (need_66_prefix) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }

    /* Emit REX prefix if needed */
    if (rex_r || rex_x || rex_b) {
        if (emit_byte(ctx, make_rex(false, rex_r, rex_x, rex_b)) < 0) return -1;
    }

    /* Emit opcode escape and opcode byte */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (need_0f38_escape) {
        if (emit_byte(ctx, 0x38) < 0) return -1;
    }
    if (emit_byte(ctx, opcode2) < 0) return -1;

    /* Emit ModR/M: dst XMM is always reg field, src is r/m field */
    if (emit_modrm_sib(ctx, dst->reg & 7, src, REG_SIZE_XMM) < 0) return -1;

    return 0;
}

/* SSE Logical Instruction Encoding
 * Supports: PAND, PANDN, POR, PXOR
 */
int encode_sse_packed_logic(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use exactly two operands: destination XMM register and source (XMM or memory)",
                      "SSE logical instruction requires 2 operands");
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    /* Determine operand types */
    bool dst_is_xmm = (dst->type == OPERAND_REG && is_xmm_reg(dst->reg));
    bool src_is_xmm = (src->type == OPERAND_REG && is_xmm_reg(src->reg));
    bool src_is_mem = (src->type == OPERAND_MEM);

    /* Validate XMM register operands use 128-bit register size metadata */
    if (!dst_is_xmm) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Use an XMM destination register, e.g. pand xmm0, xmm1.",
                      "SSE logical instruction destination must be XMM register");
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE logical instruction XMM destination must use xmm register size");
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        encoder_diagf(inst->line_number, "Encoding",
                      "XMM registers must use REG_SIZE_XMM internal size metadata",
                      "SSE logical instruction XMM source must use xmm register size");
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        encoder_diagf(inst->line_number, "Encoding",
                      "Source must be an XMM register or memory operand",
                      "SSE logical instruction source must be XMM or memory");
        return -1;
    }

    /* Determine opcode */
    bool need_66_prefix = true;  /* All logical ops need 0x66 prefix */
    uint8_t opcode2;              /* Second byte after 0x0F */

    switch (inst->type) {
        case INST_PAND:
            opcode2 = 0xDB;  /* 66 0F DB /r - PAND xmm1, xmm2/m128 */
            break;
        case INST_PANDNPD:
            opcode2 = 0xDF;  /* 66 0F DF /r - PANDN xmm1, xmm2/m128 */
            break;
        case INST_POR:
            opcode2 = 0xEB;  /* 66 0F EB /r - POR xmm1, xmm2/m128 */
            break;
        case INST_PXOR:
            opcode2 = 0xEF;  /* 66 0F EF /r - PXOR xmm1, xmm2/m128 */
            break;
        default:
            encoder_diagf(inst->line_number, "Encoding",
                          "Supported: pand, pandn, por, pxor",
                          "Unknown SSE logical instruction");
            return -1;
    }

    /* Determine REX prefix requirements */
    bool rex_r = (dst->reg >= XMM8);
    bool rex_b = false;
    bool rex_x = false;

    if (src_is_xmm && src->reg >= XMM8) {
        rex_b = true;
    } else if (src_is_mem) {
        if (src->mem.has_base && src->mem.base >= 8) rex_b = true;
        if (src->mem.has_index && src->mem.index >= 8) rex_x = true;
    }

    /* Emit instruction prefixes */
    if (need_66_prefix) {
        if (emit_byte(ctx, 0x66) < 0) return -1;
    }

    /* Emit REX prefix if needed */
    if (rex_r || rex_x || rex_b) {
        if (emit_byte(ctx, make_rex(false, rex_r, rex_x, rex_b)) < 0) return -1;
    }

    /* Emit two-byte opcode escape and second byte */
    if (emit_byte(ctx, 0x0F) < 0) return -1;
    if (emit_byte(ctx, opcode2) < 0) return -1;

    /* Emit ModR/M: dst XMM is always reg field, src is r/m field */
    if (emit_modrm_sib(ctx, dst->reg & 7, src, REG_SIZE_XMM) < 0) return -1;

    return 0;
}
