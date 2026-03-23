/**
 * x86_64 Encoder - Core instruction encoding logic
 * Generates raw machine code bytes for x86_64 instructions
 */

#include "x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
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
 * REGISTER TABLE AND PARSING
 * ============================================================================ */

typedef struct {
    const char *name;
    reg_t reg;
    reg_size_t size;
} reg_entry_t;

static const reg_entry_t reg_table[] = {
    /* 64-bit registers */
    {"rax", RAX, REG_SIZE_64}, {"rcx", RCX, REG_SIZE_64},
    {"rdx", RDX, REG_SIZE_64}, {"rbx", RBX, REG_SIZE_64},
    {"rsp", RSP, REG_SIZE_64}, {"rbp", RBP, REG_SIZE_64},
    {"rsi", RSI, REG_SIZE_64}, {"rdi", RDI, REG_SIZE_64},
    {"r8",  R8,  REG_SIZE_64}, {"r9",  R9,  REG_SIZE_64},
    {"r10", R10, REG_SIZE_64}, {"r11", R11, REG_SIZE_64},
    {"r12", R12, REG_SIZE_64}, {"r13", R13, REG_SIZE_64},
    {"r14", R14, REG_SIZE_64}, {"r15", R15, REG_SIZE_64},
    /* 32-bit registers */
    {"eax", EAX, REG_SIZE_32}, {"ecx", ECX, REG_SIZE_32},
    {"edx", EDX, REG_SIZE_32}, {"ebx", EBX, REG_SIZE_32},
    {"esp", ESP, REG_SIZE_32}, {"ebp", EBP, REG_SIZE_32},
    {"esi", ESI, REG_SIZE_32}, {"edi", EDI, REG_SIZE_32},
    {"r8d", R8D, REG_SIZE_32}, {"r9d", R9D, REG_SIZE_32},
    {"r10d", R10D, REG_SIZE_32}, {"r11d", R11D, REG_SIZE_32},
    {"r12d", R12D, REG_SIZE_32}, {"r13d", R13D, REG_SIZE_32},
    {"r14d", R14D, REG_SIZE_32}, {"r15d", R15D, REG_SIZE_32},
    /* 16-bit registers */
    {"ax", AX, REG_SIZE_16}, {"cx", CX, REG_SIZE_16},
    {"dx", DX, REG_SIZE_16}, {"bx", BX, REG_SIZE_16},
    {"sp", SP, REG_SIZE_16}, {"bp", BP, REG_SIZE_16},
    {"si", SI, REG_SIZE_16}, {"di", DI, REG_SIZE_16},
    {"r8w", R8W, REG_SIZE_16}, {"r9w", R9W, REG_SIZE_16},
    {"r10w", R10W, REG_SIZE_16}, {"r11w", R11W, REG_SIZE_16},
    {"r12w", R12W, REG_SIZE_16}, {"r13w", R13W, REG_SIZE_16},
    {"r14w", R14W, REG_SIZE_16}, {"r15w", R15W, REG_SIZE_16},
    /* 8-bit low registers */
    {"al", AL, REG_SIZE_8}, {"cl", CL, REG_SIZE_8},
    {"dl", DL, REG_SIZE_8}, {"bl", BL, REG_SIZE_8},
    {"spl", SPL, REG_SIZE_8}, {"bpl", BPL, REG_SIZE_8},
    {"sil", SIL, REG_SIZE_8}, {"dil", DIL, REG_SIZE_8},
    {"r8b", R8B, REG_SIZE_8}, {"r9b", R9B, REG_SIZE_8},
    {"r10b", R10B, REG_SIZE_8}, {"r11b", R11B, REG_SIZE_8},
    {"r12b", R12B, REG_SIZE_8}, {"r13b", R13B, REG_SIZE_8},
    {"r14b", R14B, REG_SIZE_8}, {"r15b", R15B, REG_SIZE_8},
    /* 8-bit high registers */
    {"ah", AH, REG_SIZE_8H}, {"ch", CH, REG_SIZE_8H},
    {"dh", DH, REG_SIZE_8H}, {"bh", BH, REG_SIZE_8H},
    /* Special registers */
    {"rip", RIP, REG_SIZE_64},
    /* XMM registers */
    {"xmm0", XMM0, REG_SIZE_XMM}, {"xmm1", XMM1, REG_SIZE_XMM},
    {"xmm2", XMM2, REG_SIZE_XMM}, {"xmm3", XMM3, REG_SIZE_XMM},
    {"xmm4", XMM4, REG_SIZE_XMM}, {"xmm5", XMM5, REG_SIZE_XMM},
    {"xmm6", XMM6, REG_SIZE_XMM}, {"xmm7", XMM7, REG_SIZE_XMM},
    {"xmm8", XMM8, REG_SIZE_XMM}, {"xmm9", XMM9, REG_SIZE_XMM},
    {"xmm10", XMM10, REG_SIZE_XMM}, {"xmm11", XMM11, REG_SIZE_XMM},
    {"xmm12", XMM12, REG_SIZE_XMM}, {"xmm13", XMM13, REG_SIZE_XMM},
    {"xmm14", XMM14, REG_SIZE_XMM}, {"xmm15", XMM15, REG_SIZE_XMM},
    {NULL, REG_NONE, REG_SIZE_64}
};

reg_t parse_register(const char *name, reg_size_t *size) {
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp(name, reg_table[i].name) == 0) {
            if (size) *size = reg_table[i].size;
            return reg_table[i].reg;
        }
    }
    if (size) *size = REG_SIZE_64;
    return REG_NONE;
}

const char *register_name(reg_t reg, reg_size_t size) {
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (reg_table[i].reg == reg && reg_table[i].size == size) {
            return reg_table[i].name;
        }
    }
    return "???";
}

int get_reg_size_bytes(reg_size_t size) {
    switch (size) {
        case REG_SIZE_8:
        case REG_SIZE_8H:
            return 1;
        case REG_SIZE_16:
            return 2;
        case REG_SIZE_32:
            return 4;
        case REG_SIZE_64:
            return 8;
        case REG_SIZE_XMM:
            return 16;
        default:
            return 8;
    }
}

/* ============================================================================
 * OUTPUT BUFFER OPERATIONS
 * ============================================================================ */

int emit_byte(assembler_context_t *ctx, uint8_t byte) {
    if (ctx->current_section == 1) {
        /* Write to data section */
        if (ctx->data_size >= ctx->data_capacity) {
            fprintf(stderr, "Error: Data section overflow\n");
            return -1;
        }
        ctx->data_section[ctx->data_size++] = byte;
        ctx->current_address++;
        return 0;
    }
    /* Write to text section */
    if (ctx->text_size >= ctx->text_capacity) {
        fprintf(stderr, "Error: Text section overflow\n");
        return -1;
    }
    ctx->text_section[ctx->text_size++] = byte;
    ctx->current_address++;
    return 0;
}

int emit_bytes(assembler_context_t *ctx, const uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (emit_byte(ctx, bytes[i]) < 0) return -1;
    }
    return 0;
}

int emit_word(assembler_context_t *ctx, uint16_t word) {
    uint8_t bytes[2] = { word & 0xFF, (word >> 8) & 0xFF };
    return emit_bytes(ctx, bytes, 2);
}

int emit_dword(assembler_context_t *ctx, uint32_t dword) {
    uint8_t bytes[4] = {
        dword & 0xFF,
        (dword >> 8) & 0xFF,
        (dword >> 16) & 0xFF,
        (dword >> 24) & 0xFF
    };
    return emit_bytes(ctx, bytes, 4);
}

int emit_qword(assembler_context_t *ctx, uint64_t qword) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = (qword >> (i * 8)) & 0xFF;
    }
    return emit_bytes(ctx, bytes, 8);
}

int emit_le16(assembler_context_t *ctx, int16_t val) {
    return emit_word(ctx, (uint16_t)val);
}

int emit_le32(assembler_context_t *ctx, int32_t val) {
    return emit_dword(ctx, (uint32_t)val);
}

int emit_le64(assembler_context_t *ctx, int64_t val) {
    return emit_qword(ctx, (uint64_t)val);
}

/* ============================================================================
 * REX PREFIX
 * ============================================================================ */

/* Check if an operand is a high 8-bit register (AH, BH, CH, DH) */
static int is_high_8bit_reg(const operand_t *op) {
    return op && op->type == OPERAND_REG && op->reg_size == REG_SIZE_8H;
}

/* Check if an operand requires REX prefix (R8-R15, SPL/BPL/SIL/DIL) */
static int operand_needs_rex(const operand_t *op) {
    if (!op) return 0;
    if (op->type == OPERAND_REG) {
        if (op->reg >= 8) return 1;
        if (op->reg_size == REG_SIZE_8 && op->reg >= 4) return 1;
        /* REG_SIZE_8H (AH/BH/CH/DH) does NOT need REX */
    } else if (op->type == OPERAND_MEM) {
        if (op->mem.base >= 8) return 1;
        if (op->mem.has_index && op->mem.index >= 8) return 1;
    }
    return 0;
}

/* Check for invalid combination: high 8-bit reg with REX-requiring operand */
static int check_rex_conflict(const parsed_instruction_t *inst, int op1_idx, int op2_idx) {
    const operand_t *op1 = (op1_idx >= 0 && op1_idx < inst->operand_count) ? &inst->operands[op1_idx] : NULL;
    const operand_t *op2 = (op2_idx >= 0 && op2_idx < inst->operand_count) ? &inst->operands[op2_idx] : NULL;
    
    int has_high_8bit = is_high_8bit_reg(op1) || is_high_8bit_reg(op2);
    int needs_rex = operand_needs_rex(op1) || operand_needs_rex(op2);
    
    return has_high_8bit && needs_rex;
}

int needs_rex(const operand_t *op) {
    if (op->type == OPERAND_REG) {
        /* REX needed for R8-R15, SPL/BPL/SIL/DIL (low 8-bit regs 4-7) */
        /* AH/BH/CH/DH (REG_SIZE_8H) do NOT use REX - they use legacy encoding */
        if (op->reg >= 8) return 1;
        if (op->reg_size == REG_SIZE_8 && op->reg >= 4) return 1;
        /* Note: REG_SIZE_8H (AH/BH/CH/DH) return 0 - no REX needed */
    } else if (op->type == OPERAND_MEM) {
        if (op->mem.base >= 8 || op->mem.index >= 8) return 1;
    }
    return 0;
}

uint8_t make_rex(bool w, bool r, bool x, bool b) {
    return 0x40 | (w ? 0x08 : 0) | (r ? 0x04 : 0) | (x ? 0x02 : 0) | (b ? 0x01 : 0);
}

int emit_rex(assembler_context_t *ctx, bool w, const operand_t *reg_op,
                    const operand_t *rm_op, const operand_t *sib_index_op) {
    bool r = false, x = false, b = false;

    if (reg_op && reg_op->type == OPERAND_REG && reg_op->reg >= 8) {
        r = true;
    }

    if (sib_index_op && sib_index_op->type == OPERAND_REG && sib_index_op->reg >= 8) {
        x = true;
    }

    if (rm_op) {
        if (rm_op->type == OPERAND_REG && rm_op->reg >= 8) {
            b = true;
        } else if (rm_op->type == OPERAND_MEM) {
            if (rm_op->mem.has_base && rm_op->mem.base >= 8) b = true;
            if (rm_op->mem.has_index && rm_op->mem.index >= 8) x = true;
        }
    }

    uint8_t rex = make_rex(w, r, x, b);

    /* Don't emit REX if not needed, unless we need 64-bit operand size */
    if (!w && !r && !x && !b) return 0;

    return emit_byte(ctx, rex);
}

/* ============================================================================
 * MODR/M AND SIB ENCODING
 * ============================================================================ */

uint8_t make_modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
    return ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7);
}

uint8_t make_sib(uint8_t scale, uint8_t index, uint8_t base) {
    uint8_t scale_bits = (scale == 8) ? 3 : (scale == 4) ? 2 : (scale == 2) ? 1 : 0;
    return (scale_bits << 6) | ((index & 7) << 3) | (base & 7);
}

/* Check if displacement fits in signed 8-bit */
static int fits_in_int8(int32_t val) {
    return val >= -128 && val <= 127;
}

/* Forward declaration for add_fixup */
extern int add_fixup(assembler_context_t *ctx, const char *label, uint64_t location,
                     int size, instruction_type_t inst_type);
extern int add_fixup_rip_relative(assembler_context_t *ctx, const char *label,
                                   uint64_t location, int section);

int emit_modrm_sib(assembler_context_t *ctx, uint8_t reg_opcode,
                   const operand_t *operand, reg_size_t size) {
    (void)size;

    if (operand->type == OPERAND_REG) {
        /* Register direct: mod=3, rm=reg */
        uint8_t modrm = make_modrm(3, reg_opcode, operand->reg & 7);
        return emit_byte(ctx, modrm);
    }

    if (operand->type != OPERAND_MEM) {
        fprintf(stderr, "Error: Expected memory operand\n");
        return -1;
    }

    const mem_operand_t *mem = &operand->mem;

    if (mem->has_index && !(mem->scale == 1 || mem->scale == 2 || mem->scale == 4 || mem->scale == 8)) {
        fprintf(stderr, "Error: Invalid scale factor %d (expected 1, 2, 4, or 8)\n", mem->scale);
        return -1;
    }

    if (mem->is_rip_relative && (mem->has_index || mem->has_base)) {
        fprintf(stderr, "Error: RIP-relative addressing cannot combine base/index registers\n");
        return -1;
    }

    /* RIP-relative addressing: mod=00, r/m=101 with 32-bit displacement */
    if (mem->is_rip_relative) {
        /* RIP-relative: [RIP + disp32] - mod=00, r/m=101 */
        uint8_t modrm = make_modrm(0, reg_opcode, 5); /* 5 = RIP-relative indicator */
        if (emit_byte(ctx, modrm) < 0) return -1;
        
        /* Emit displacement - either immediate value or add fixup for label */
        if (mem->label[0] != '\0') {
            /* Label reference - add fixup for RIP-relative addressing */
            if (add_fixup_rip_relative(ctx, mem->label, ctx->current_address, ctx->current_section) < 0) {
                return -1;
            }
            /* Emit placeholder (0) for now */
            if (emit_le32(ctx, 0) < 0) return -1;
        } else {
            /* Just a displacement */
            if (emit_le32(ctx, mem->displacement) < 0) return -1;
        }
        return 0;
    }

    /* Simple [base] or [base+disp] addressing */
    if (!mem->has_index) {
        uint8_t base_reg = mem->has_base ? (mem->base & 7) : 5; /* 5 = disp32 only */

        if (!mem->has_displacement && mem->has_base && base_reg != 4 && base_reg != 5) {
            /* [base] - mod=0, no displacement (except RBP which needs disp8=0) */
            uint8_t modrm = make_modrm(0, reg_opcode, base_reg);
            if (emit_byte(ctx, modrm) < 0) return -1;
        } else if (!mem->has_displacement && mem->has_base && base_reg == 5) {
            /* [RBP] requires disp8=0 */
            uint8_t modrm = make_modrm(1, reg_opcode, 5);
            if (emit_byte(ctx, modrm) < 0) return -1;
            if (emit_byte(ctx, 0) < 0) return -1;
        } else if (mem->has_displacement && fits_in_int8(mem->displacement)) {
            /* [base+disp8] */
            uint8_t modrm = make_modrm(1, reg_opcode, base_reg);
            if (emit_byte(ctx, modrm) < 0) return -1;
            if (base_reg == 4) {
                /* Need SIB byte for RSP/R12 */
                uint8_t sib = make_sib(1, 4, base_reg);
                if (emit_byte(ctx, sib) < 0) return -1;
            }
            if (emit_byte(ctx, (uint8_t)(int8_t)mem->displacement) < 0) return -1;
        } else {
            /* [base+disp32] or [disp32] */
            uint8_t modrm = make_modrm(mem->has_base ? 2 : 0, reg_opcode, base_reg);
            if (emit_byte(ctx, modrm) < 0) return -1;
            if (base_reg == 4 && mem->has_base) {
                /* Need SIB byte for RSP/R12 */
                uint8_t sib = make_sib(1, 4, base_reg);
                if (emit_byte(ctx, sib) < 0) return -1;
            }
            if (emit_le32(ctx, mem->displacement) < 0) return -1;
        }
    } else {
        /* [base + index*scale + disp] - requires SIB */
        uint8_t base_reg = mem->has_base ? (mem->base & 7) : 5;
        uint8_t index_reg = mem->index & 7;

        /* Special case: RBP/R13 as base with mod=0 needs disp8=0 */
        bool need_sib_disp = (!mem->has_base || base_reg == 5) && mem->displacement == 0;

        if (mem->has_displacement && fits_in_int8(mem->displacement) && !need_sib_disp) {
            /* disp8 */
            uint8_t modrm = make_modrm(1, reg_opcode, 4); /* 4 = SIB follows */
            uint8_t sib = make_sib(mem->scale, index_reg, base_reg);
            if (emit_byte(ctx, modrm) < 0) return -1;
            if (emit_byte(ctx, sib) < 0) return -1;
            if (emit_byte(ctx, (uint8_t)(int8_t)mem->displacement) < 0) return -1;
        } else {
            /* disp32 or no base */
            uint8_t mod = (mem->has_displacement || !mem->has_base) ? 2 : 0;
            uint8_t modrm = make_modrm(mod, reg_opcode, 4); /* 4 = SIB follows */
            uint8_t sib = make_sib(mem->scale, index_reg, base_reg);
            if (emit_byte(ctx, modrm) < 0) return -1;
            if (emit_byte(ctx, sib) < 0) return -1;
            if (mod != 0 || !mem->has_base) {
                if (emit_le32(ctx, mem->displacement) < 0) return -1;
            }
        }
    }

    return 0;
}

/* ============================================================================
 * INSTRUCTION ENCODING
 * ============================================================================ */

/* MOV instruction encoding */
int encode_mov(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: mov requires 2 operands at line %d\n", inst->line_number);
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

        if (is_64bit && (src->immediate < INT32_MIN || src->immediate > INT32_MAX)) {
            /* MOV r64, imm64 - special encoding: REX.W + C7 /0 followed by imm64,
             * but actually the shorter form for certain regs uses B8+rq */
            uint8_t opcode = 0xB8 + reg_val;
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
            if (emit_byte(ctx, 0xB8 + reg_val) < 0) return -1;
            if (emit_le32(ctx, (int32_t)src->immediate) < 0) return -1;
        } else if (size == REG_SIZE_16) {
            /* MOV r16, imm16 - need 0x66 prefix */
            if (emit_byte(ctx, 0x66) < 0) return -1;
            if (need_rex_prefix) {
                if (emit_rex(ctx, false, dst, NULL, NULL) < 0) return -1;
            }
            if (emit_byte(ctx, 0xB8 + reg_val) < 0) return -1;
            if (emit_le16(ctx, (int16_t)src->immediate) < 0) return -1;
        } else {
            /* MOV r8, imm8 */
            if (dst->reg_size == REG_SIZE_8H) {
                /* AH, BH, CH, DH - no REX allowed */
                if (emit_byte(ctx, 0xB0 + reg_val) < 0) return -1;
            } else {
                if (emit_rex(ctx, false, dst, NULL, NULL) < 0) return -1;
                if (emit_byte(ctx, 0xB0 + reg_val) < 0) return -1;
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

    fprintf(stderr, "Error: Unsupported mov operand combination at line %d\n", inst->line_number);
    return -1;
}

/* PUSH/POP encoding */
int encode_push_pop(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: %s requires 1 operand at line %d\n",
                inst->type == INST_PUSH ? "push" : "pop", inst->line_number);
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
        uint8_t opcode = is_push ? (0x50 + reg) : (0x58 + reg);
        if (emit_byte(ctx, opcode) < 0) return -1;
        return 0;
    }

    if (op->type == OPERAND_IMM) {
        if (!is_push) {
            fprintf(stderr, "Error: pop with immediate not valid\n");
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

    fprintf(stderr, "Error: Unsupported push/pop operand at line %d\n", inst->line_number);
    return -1;
}

/* Arithmetic instructions (ADD, SUB, AND, OR, XOR, CMP) */
int encode_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: Arithmetic instruction requires 2 operands at line %d\n", inst->line_number);
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
    switch (inst->type) {
        case INST_ADD: reg_opcode = 0; break;
        case INST_OR:  reg_opcode = 1; break;
        case INST_ADC: reg_opcode = 2; break;
        case INST_SBB: reg_opcode = 3; break;
        case INST_AND: reg_opcode = 4; break;
        case INST_SUB: reg_opcode = 5; break;
        case INST_XOR: reg_opcode = 6; break;
        case INST_CMP: reg_opcode = 7; break;
        default:
            fprintf(stderr, "Error: Unknown arithmetic instruction\n");
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
            uint8_t opcode = 0x05 + (reg_opcode << 3);
            if (is_64bit && emit_rex(ctx, true, dst, NULL, NULL) < 0) return -1;
            if (emit_byte(ctx, opcode) < 0) return -1;
            if (emit_le32(ctx, imm32) < 0) return -1;
            return 0;
        }

        /* For 64-bit operations, if value doesn't fit in signed 32-bit,
         * we need to load it into a register first. This is because all
         * immediate forms sign-extend to 64 bits. */
        if (is_64bit && !fits_signed32) {
            fprintf(stderr, "Error: 64-bit immediate 0x%llx out of range for %s, "
                    "use mov reg, imm64 first at line %d\n",
                    (unsigned long long)imm64,
                    inst->type == INST_CMP ? "cmp" :
                    inst->type == INST_ADD ? "add" :
                    inst->type == INST_SUB ? "sub" : "op",
                    inst->line_number);
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
        uint8_t opcode = 0x01 + (reg_opcode << 3);
        if (emit_rex(ctx, is_64bit, src, dst, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, src->reg & 7, dst, size) < 0) return -1;
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP reg, mem */
    if (dst->type == OPERAND_REG && src->type == OPERAND_MEM) {
        uint8_t opcode = 0x03 + (reg_opcode << 3); /* 03 /r (ADD), 0B /r (OR), etc */
        if (emit_rex(ctx, is_64bit, dst, src, NULL) < 0) return -1;
        if (emit_byte(ctx, opcode) < 0) return -1;
        if (emit_modrm_sib(ctx, dst->reg & 7, src, size) < 0) return -1;
        return 0;
    }

    /* ADD/SUB/AND/OR/XOR/CMP mem, reg */
    if (dst->type == OPERAND_MEM && src->type == OPERAND_REG) {
        uint8_t opcode = 0x01 + (reg_opcode << 3);
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

    fprintf(stderr, "Error: Unsupported arithmetic operand combination at line %d\n", inst->line_number);
    return -1;
}

/* INC/DEC/NEG/NOT encoding */
int encode_unary_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 1) {
        fprintf(stderr, "Error: Unary instruction requires 1 operand at line %d\n", inst->line_number);
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
            fprintf(stderr, "Error: Unknown unary instruction\n");
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

    fprintf(stderr, "Error: Unsupported unary operand at line %d\n", inst->line_number);
    return -1;
}

/* LEA encoding */
int encode_lea(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    if (inst->operand_count != 2) {
        fprintf(stderr, "Error: lea requires 2 operands at line %d\n", inst->line_number);
        return -1;
    }

    const operand_t *dst = &inst->operands[0];
    const operand_t *src = &inst->operands[1];

    if (dst->type != OPERAND_REG || src->type != OPERAND_MEM) {
        fprintf(stderr, "Error: lea requires register destination and memory source\n");
        return -1;
    }

    reg_size_t size = dst->reg_size;
    if (size == REG_SIZE_8 || size == REG_SIZE_8H) {
        fprintf(stderr, "Error: lea destination must be 16, 32, or 64-bit register\n");
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
        fprintf(stderr, "Error: String instruction takes no operands at line %d\n", inst->line_number);
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
            fprintf(stderr, "Error: Unknown string instruction at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE move XMM destination must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE move XMM source must use xmm register size at line %d\n",
                inst->line_number);
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
            fprintf(stderr, "Error: Unknown SSE move instruction at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE arithmetic destination must use xmm register size at line %d\n",
                inst->line_number);
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
        fprintf(stderr, "Error: SSE arithmetic XMM source must use xmm register size at line %d\n",
                inst->line_number);
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
            fprintf(stderr, "Error: Unknown SSE arithmetic instruction at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE packed arithmetic requires 2 operands at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE packed arithmetic destination must be XMM register at line %d\n",
                inst->line_number);
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE packed arithmetic XMM destination must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE packed arithmetic XMM source must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        fprintf(stderr, "Error: SSE packed arithmetic source must be XMM or memory at line %d\n",
                inst->line_number);
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
            fprintf(stderr, "Error: Unknown SSE packed arithmetic instruction at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE packed compare requires 2 operands at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE packed compare destination must be XMM register at line %d\n",
                inst->line_number);
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE packed compare XMM destination must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE packed compare XMM source must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        fprintf(stderr, "Error: SSE packed compare source must be XMM or memory at line %d\n",
                inst->line_number);
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
            fprintf(stderr, "Error: Unknown SSE packed compare instruction at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE logical instruction requires 2 operands at line %d\n", inst->line_number);
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
        fprintf(stderr, "Error: SSE logical instruction destination must be XMM register at line %d\n",
                inst->line_number);
        return -1;
    }
    if (dst->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE logical instruction XMM destination must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }
    if (src_is_xmm && src->reg_size != REG_SIZE_XMM) {
        fprintf(stderr, "Error: SSE logical instruction XMM source must use xmm register size at line %d\n",
                inst->line_number);
        return -1;
    }

    /* Validate source is XMM or memory */
    if (!src_is_xmm && !src_is_mem) {
        fprintf(stderr, "Error: SSE logical instruction source must be XMM or memory at line %d\n",
                inst->line_number);
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
            fprintf(stderr, "Error: Unknown SSE logical instruction at line %d\n", inst->line_number);
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
