/**
 * x86_64 Encoding Core - shared low-level encoding primitives
 * Register lookup, byte emitters, and ModRM/SIB/REX helpers.
 */

#include "x86_64_asm/x86_64_asm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

reg_t
parse_register(const char *name, reg_size_t *size)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp(name, reg_table[i].name) == 0) {
            if (size) {
                *size = reg_table[i].size;
            }
            return reg_table[i].reg;
        }
    }

    if (size) {
        *size = REG_SIZE_64;
    }
    return REG_NONE;
}

const char *
register_name(reg_t reg, reg_size_t size)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (reg_table[i].reg == reg && reg_table[i].size == size) {
            return reg_table[i].name;
        }
    }

    return "???";
}

int
get_reg_size_bytes(reg_size_t size)
{
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

int
emit_byte(assembler_context_t *ctx, uint8_t byte)
{
    if (ctx->current_section == 1) {
        if (ctx->data_size >= ctx->data_capacity) {
            fprintf(stderr,
                    "Error at line 1, column 1: [Memory] Data section overflow\n"
                    "Suggestion: Increase data capacity or reduce emitted data size.\n");
            return -1;
        }
        ctx->data_section[ctx->data_size++] = byte;
        ctx->current_address++;
        listing_emit_byte(ctx, byte);
        return 0;
    }

    if (ctx->text_size >= ctx->text_capacity) {
        fprintf(stderr,
                "Error at line 1, column 1: [Memory] Text section overflow\n"
                "Suggestion: Increase text capacity or reduce emitted instruction bytes.\n");
        return -1;
    }

    ctx->text_section[ctx->text_size++] = byte;
    ctx->current_address++;
    listing_emit_byte(ctx, byte);
    return 0;
}

void
listing_start_instruction(assembler_context_t *ctx, int line_number, const char *source)
{
    listing_entry_t *entry;
    int new_capacity;

    if (!ctx || !ctx->emit_listing) {
        return;
    }

    if (ctx->listing_count >= ctx->listing_capacity) {
        new_capacity = (ctx->listing_capacity > 0) ? (ctx->listing_capacity * 2) : 128;
        listing_entry_t *grown = realloc(ctx->listing_entries,
                                         (size_t)new_capacity * sizeof(*grown));
        if (!grown) {
            return;
        }
        ctx->listing_entries = grown;
        ctx->listing_capacity = new_capacity;
    }

    entry = &ctx->listing_entries[ctx->listing_count];
    memset(entry, 0, sizeof(*entry));
    entry->line_number = line_number > 0 ? line_number : 1;
    entry->address = ctx->current_address;
    if (source && source[0] != '\0') {
        strncpy(entry->source_line, source, sizeof(entry->source_line) - 1);
        entry->source_line[sizeof(entry->source_line) - 1] = '\0';
    }

    ctx->listing_active_index = ctx->listing_count;
    ctx->listing_active = true;
    ctx->listing_count++;
}

void
listing_emit_byte(assembler_context_t *ctx, uint8_t byte)
{
    listing_entry_t *entry;

    if (!ctx || !ctx->emit_listing || !ctx->listing_active) {
        return;
    }
    if (ctx->listing_active_index < 0 || ctx->listing_active_index >= ctx->listing_count) {
        return;
    }

    entry = &ctx->listing_entries[ctx->listing_active_index];
    if (entry->byte_count < (int)sizeof(entry->bytes)) {
        entry->bytes[entry->byte_count++] = byte;
    }
}

void
listing_end_instruction(assembler_context_t *ctx)
{
    int idx;

    if (!ctx || !ctx->emit_listing) {
        return;
    }

    if (!ctx->listing_active) {
        return;
    }

    idx = ctx->listing_active_index;
    if (idx >= 0 && idx < ctx->listing_count) {
        if (ctx->listing_entries[idx].byte_count == 0 && idx == (ctx->listing_count - 1)) {
            ctx->listing_count--;
        }
    }

    ctx->listing_active = false;
    ctx->listing_active_index = -1;
}

int
emit_bytes(assembler_context_t *ctx, const uint8_t *bytes, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (emit_byte(ctx, bytes[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

int
emit_word(assembler_context_t *ctx, uint16_t word)
{
    uint8_t bytes[2] = {
        (uint8_t)(word & 0xFFu),
        (uint8_t)((word >> 8) & 0xFFu)
    };
    return emit_bytes(ctx, bytes, 2);
}

int
emit_dword(assembler_context_t *ctx, uint32_t dword)
{
    uint8_t bytes[4] = {
        (uint8_t)(dword & 0xFFu),
        (uint8_t)((dword >> 8) & 0xFFu),
        (uint8_t)((dword >> 16) & 0xFFu),
        (uint8_t)((dword >> 24) & 0xFFu)
    };
    return emit_bytes(ctx, bytes, 4);
}

int
emit_qword(assembler_context_t *ctx, uint64_t qword)
{
    uint8_t bytes[8];

    for (int i = 0; i < 8; i++) {
        bytes[i] = (uint8_t)((qword >> (i * 8)) & 0xFFu);
    }

    return emit_bytes(ctx, bytes, 8);
}

int
emit_le16(assembler_context_t *ctx, int16_t val)
{
    return emit_word(ctx, (uint16_t)val);
}

int
emit_le32(assembler_context_t *ctx, int32_t val)
{
    return emit_dword(ctx, (uint32_t)val);
}

int
emit_le64(assembler_context_t *ctx, int64_t val)
{
    return emit_qword(ctx, (uint64_t)val);
}

int
needs_rex(const operand_t *op)
{
    if (op->type == OPERAND_REG) {
        if (op->reg >= 8) {
            return 1;
        }
        if (op->reg_size == REG_SIZE_8 && op->reg >= 4) {
            return 1;
        }
    } else if (op->type == OPERAND_MEM) {
        if (op->mem.base >= 8 || op->mem.index >= 8) {
            return 1;
        }
    }

    return 0;
}

uint8_t
make_rex(bool w, bool r, bool x, bool b)
{
    return (uint8_t)(0x40u | (w ? 0x08u : 0u) | (r ? 0x04u : 0u) |
                     (x ? 0x02u : 0u) | (b ? 0x01u : 0u));
}

int
emit_rex(assembler_context_t *ctx, bool w, const operand_t *reg_op,
         const operand_t *rm_op, const operand_t *sib_index_op)
{
    bool r = false;
    bool x = false;
    bool b = false;

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
            if (rm_op->mem.has_base && rm_op->mem.base >= 8) {
                b = true;
            }
            if (rm_op->mem.has_index && rm_op->mem.index >= 8) {
                x = true;
            }
        }
    }

    if (!w && !r && !x && !b) {
        return 0;
    }

    return emit_byte(ctx, make_rex(w, r, x, b));
}

uint8_t
make_modrm(uint8_t mod, uint8_t reg, uint8_t rm)
{
    return (uint8_t)(((mod & 3u) << 6) | ((reg & 7u) << 3) | (rm & 7u));
}

uint8_t
make_sib(uint8_t scale, uint8_t index, uint8_t base)
{
    uint8_t scale_bits = (scale == 8u) ? 3u : (scale == 4u) ? 2u : (scale == 2u) ? 1u : 0u;
    uint32_t sib = ((uint32_t)scale_bits << 6)
                 | (((uint32_t)index & 7u) << 3)
                 | ((uint32_t)base & 7u);
    return (uint8_t)sib;
}

int
fits_in_int8(int32_t val)
{
    return val >= -128 && val <= 127;
}

int
emit_modrm_sib(assembler_context_t *ctx, uint8_t reg_opcode,
               const operand_t *operand, reg_size_t size)
{
    (void)size;

    if (operand->type == OPERAND_REG) {
        return emit_byte(ctx, make_modrm(3, reg_opcode, (uint8_t)(operand->reg & 7)));
    }

    if (operand->type != OPERAND_MEM) {
        fprintf(stderr,
                "Error at line 1, column 1: [Operand] Expected memory operand\n"
                "Suggestion: Use memory syntax like [rax], [rbp-8], or [rip+disp].\n");
        return -1;
    }

    const mem_operand_t *mem = &operand->mem;

    if (mem->has_index && !(mem->scale == 1 || mem->scale == 2 || mem->scale == 4 || mem->scale == 8)) {
        fprintf(stderr,
                "Error at line 1, column 1: [Operand] Invalid scale factor %d (expected 1, 2, 4, or 8)\n"
                "Suggestion: Use only legal SIB scale values: 1, 2, 4, or 8.\n",
                mem->scale);
        return -1;
    }

    if (mem->is_rip_relative && (mem->has_index || mem->has_base)) {
        fprintf(stderr,
                "Error at line 1, column 1: [Operand] RIP-relative addressing cannot combine base/index registers\n"
                "Suggestion: Use either [rip+disp] or a base/index form, but not both together.\n");
        return -1;
    }

    if (mem->is_rip_relative) {
        if (emit_byte(ctx, make_modrm(0, reg_opcode, 5)) < 0) {
            return -1;
        }

        if (mem->label[0] != '\0') {
            if (add_fixup_rip_relative(ctx, mem->label, ctx->current_address, ctx->current_section) < 0) {
                return -1;
            }
            return emit_le32(ctx, mem->displacement);
        }

        return emit_le32(ctx, mem->displacement);
    }

    /* Absolute disp32 addressing (no base/index): ModRM rm=100 + SIB base=101. */
    if (!mem->has_base && !mem->has_index) {
        if (emit_byte(ctx, make_modrm(0, reg_opcode, 4)) < 0) {
            return -1;
        }
        if (emit_byte(ctx, make_sib(1, 4, 5)) < 0) {
            return -1;
        }
        if (mem->label[0] != '\0') {
            if (add_fixup(ctx, mem->label, ctx->current_address, 4, INST_MOV) < 0) {
                return -1;
            }
        }
        return emit_le32(ctx, mem->displacement);
    }

    if (!mem->has_index) {
        uint8_t base_reg = (uint8_t)(mem->has_base ? (mem->base & 7) : 5);

        if (!mem->has_displacement && mem->has_base && base_reg != 4 && base_reg != 5) {
            return emit_byte(ctx, make_modrm(0, reg_opcode, base_reg));
        }

        if (!mem->has_displacement && mem->has_base && base_reg == 5) {
            if (emit_byte(ctx, make_modrm(1, reg_opcode, 5)) < 0) {
                return -1;
            }
            return emit_byte(ctx, 0);
        }

        if (mem->has_displacement && fits_in_int8(mem->displacement)) {
            if (emit_byte(ctx, make_modrm(1, reg_opcode, base_reg)) < 0) {
                return -1;
            }
            if (base_reg == 4) {
                if (emit_byte(ctx, make_sib(1, 4, base_reg)) < 0) {
                    return -1;
                }
            }
            return emit_byte(ctx, (uint8_t)(int8_t)mem->displacement);
        }

        if (emit_byte(ctx, make_modrm((uint8_t)(mem->has_base ? 2 : 0), reg_opcode, base_reg)) < 0) {
            return -1;
        }
        if (base_reg == 4 && mem->has_base) {
            if (emit_byte(ctx, make_sib(1, 4, base_reg)) < 0) {
                return -1;
            }
        }
        return emit_le32(ctx, mem->displacement);
    }

    uint8_t base_reg = (uint8_t)(mem->has_base ? (mem->base & 7) : 5);
    uint8_t index_reg = (uint8_t)(mem->index & 7);
    bool need_sib_disp = (!mem->has_base || base_reg == 5) && mem->displacement == 0;

    if (mem->has_displacement && fits_in_int8(mem->displacement) && !need_sib_disp) {
        if (emit_byte(ctx, make_modrm(1, reg_opcode, 4)) < 0) {
            return -1;
        }
        if (emit_byte(ctx, make_sib((uint8_t)mem->scale, index_reg, base_reg)) < 0) {
            return -1;
        }
        return emit_byte(ctx, (uint8_t)(int8_t)mem->displacement);
    }

    uint8_t mod = (uint8_t)((mem->has_displacement || !mem->has_base) ? 2 : 0);
    if (emit_byte(ctx, make_modrm(mod, reg_opcode, 4)) < 0) {
        return -1;
    }
    if (emit_byte(ctx, make_sib((uint8_t)mem->scale, index_reg, base_reg)) < 0) {
        return -1;
    }
    if (mod != 0 || !mem->has_base) {
        return emit_le32(ctx, mem->displacement);
    }

    return 0;
}
