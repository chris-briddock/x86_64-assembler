/**
 * x86_64 Disassembler - baseline implementation
 */

#include "x86_64_asm/x86_64_asm.h"
#include "x86_64_asm/opcode_lookup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIS_MAX_INST_BYTES 15

typedef struct {
    unsigned char e_ident[16];
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
} dis_elf64_ehdr_t;

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
} dis_elf64_shdr_t;

typedef struct {
    const uint8_t *code;
    size_t code_size;
    uint64_t base_address;
    size_t offset;
} dis_ctx_t;

typedef struct {
    int consumed;
    int byte_count;
    uint8_t bytes[DIS_MAX_INST_BYTES];
    char text[192];
} dis_decoded_t;

static const char *reg64_name(int reg_code);
static const char *reg32_name(int reg_code);
static const char *reg16_name(int reg_code);
static const char *reg8_name(int reg_code, int rex_present);
static const char *select_reg_name(int reg_code, int size_bits, int rex_present);
static int read_le32(const uint8_t *buf, size_t size, size_t pos, uint32_t *out);
static int read_le64(const uint8_t *buf, size_t size, size_t pos, uint64_t *out);
static int append_with_sign(char *dst, size_t dst_size, int64_t value);
static int format_rm_operand(const uint8_t *code, size_t size, size_t modrm_pos,
                             uint8_t rex, int size_bits,
                             char *out, size_t out_size,
                             int *rm_len);
static int decode_one(dis_ctx_t *ctx, dis_decoded_t *out);

static const char *reg64_name(int reg_code) {
    static const char *regs[16] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    if (reg_code < 0 || reg_code > 15) {
        return "?";
    }
    return regs[reg_code];
}

static const char *reg32_name(int reg_code) {
    static const char *regs[16] = {
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
    };

    if (reg_code < 0 || reg_code > 15) {
        return "?";
    }
    return regs[reg_code];
}

static const char *reg16_name(int reg_code) {
    static const char *regs[16] = {
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
    };

    if (reg_code < 0 || reg_code > 15) {
        return "?";
    }
    return regs[reg_code];
}

static const char *reg8_name(int reg_code, int rex_present) {
    static const char *regs_rex[16] = {
        "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
    };
    static const char *regs_no_rex[16] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
    };

    if (reg_code < 0 || reg_code > 15) {
        return "?";
    }
    return rex_present ? regs_rex[reg_code] : regs_no_rex[reg_code];
}

static const char *select_reg_name(int reg_code, int size_bits, int rex_present) {
    if (size_bits == 8) {
        return reg8_name(reg_code, rex_present);
    }
    if (size_bits == 16) {
        return reg16_name(reg_code);
    }
    if (size_bits == 32) {
        return reg32_name(reg_code);
    }
    return reg64_name(reg_code);
}

static int read_le32(const uint8_t *buf, size_t size, size_t pos, uint32_t *out) {
    uint32_t value;

    if (!buf || !out) {
        return -1;
    }
    if (pos + 4U > size) {
        return -1;
    }

    value = (uint32_t)buf[pos] |
            ((uint32_t)buf[pos + 1U] << 8) |
            ((uint32_t)buf[pos + 2U] << 16) |
            ((uint32_t)buf[pos + 3U] << 24);
    *out = value;
    return 0;
}

static int read_le64(const uint8_t *buf, size_t size, size_t pos, uint64_t *out) {
    uint64_t value = 0;

    if (!buf || !out) {
        return -1;
    }
    if (pos + 8U > size) {
        return -1;
    }

    for (size_t i = 0; i < 8U; i++) {
        value |= ((uint64_t)buf[pos + i] << (8U * i));
    }
    *out = value;
    return 0;
}

static int append_with_sign(char *dst, size_t dst_size, int64_t value) {
    int written;

    if (value < 0) {
        written = snprintf(dst, dst_size, "-0x%llx", (unsigned long long)(-value));
    } else if (value > 0) {
        written = snprintf(dst, dst_size, "+0x%llx", (unsigned long long)value);
    } else {
        written = snprintf(dst, dst_size, "+0x0");
    }

    if (written < 0 || (size_t)written >= dst_size) {
        return -1;
    }
    return 0;
}

static int format_rm_operand(const uint8_t *code, size_t size, size_t modrm_pos,
                             uint8_t rex, int size_bits,
                             char *out, size_t out_size,
                             int *rm_len) {
    uint8_t modrm;
    int mod;
    int rm;
    int rex_b;
    int rex_x;

    if (!code || !out || !rm_len) {
        return -1;
    }
    if (modrm_pos >= size) {
        return -1;
    }

    modrm = code[modrm_pos];
    mod = (modrm >> 6) & 0x3;
    rm = modrm & 0x7;
    rex_b = (rex >> 0) & 0x1;
    rex_x = (rex >> 1) & 0x1;

    if (mod == 3) {
        const char *reg_name = select_reg_name(rm + (rex_b << 3), size_bits, rex != 0);
        if (snprintf(out, out_size, "%s", reg_name) < 0) {
            return -1;
        }
        *rm_len = 1;
        return 0;
    }

    {
        size_t cursor = modrm_pos + 1U;
        int has_sib = (rm == 4);
        int disp_size = 0;
        int base_code = rm;
        int index_code = -1;
        int scale = 1;
        int has_base = 1;
        int is_rip_relative = 0;
        int32_t disp = 0;
        char operand[128];
        int written;

        if (has_sib) {
            uint8_t sib;
            int sib_base_raw;
            int sib_index_raw;
            int scale_bits;

            if (cursor >= size) {
                return -1;
            }

            sib = code[cursor++];
            scale_bits = (sib >> 6) & 0x3;
            sib_index_raw = (sib >> 3) & 0x7;
            sib_base_raw = sib & 0x7;

            scale = 1 << scale_bits;

            if (sib_index_raw != 4) {
                index_code = sib_index_raw + (rex_x << 3);
            }

            base_code = sib_base_raw + (rex_b << 3);
            if (mod == 0 && sib_base_raw == 5) {
                has_base = 0;
                disp_size = 4;
            }
        } else {
            base_code = rm + (rex_b << 3);
            if (mod == 0 && rm == 5) {
                is_rip_relative = 1;
                disp_size = 4;
            }
        }

        if (mod == 1) {
            disp_size = 1;
        } else if (mod == 2) {
            disp_size = 4;
        }

        if (disp_size == 1) {
            if (cursor >= size) {
                return -1;
            }
            disp = (int8_t)code[cursor++];
        } else if (disp_size == 4) {
            uint32_t raw_disp;
            if (read_le32(code, size, cursor, &raw_disp) < 0) {
                return -1;
            }
            disp = (int32_t)raw_disp;
            cursor += 4U;
        }

        written = snprintf(operand, sizeof(operand), "[");
        if (written < 0 || (size_t)written >= sizeof(operand)) {
            return -1;
        }

        if (is_rip_relative) {
            char disp_buf[32];
            if (append_with_sign(disp_buf, sizeof(disp_buf), disp) < 0) {
                return -1;
            }
            written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                               "rip%s", disp_buf);
            if (written < 0 || (size_t)written >= sizeof(operand) - strlen(operand)) {
                return -1;
            }
        } else {
            int needs_plus = 0;

            if (has_base) {
                written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                                   "%s", reg64_name(base_code));
                if (written < 0 || (size_t)written >= sizeof(operand) - strlen(operand)) {
                    return -1;
                }
                needs_plus = 1;
            }

            if (index_code >= 0) {
                written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                                   "%s%s*%d", needs_plus ? "+" : "", reg64_name(index_code), scale);
                if (written < 0 || (size_t)written >= sizeof(operand) - strlen(operand)) {
                    return -1;
                }
                needs_plus = 1;
            }

            if (disp_size > 0 || (!has_base && index_code < 0)) {
                if (!needs_plus) {
                    written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                                       "0x%llx", (unsigned long long)(uint32_t)disp);
                } else if (disp < 0) {
                    written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                                       "-0x%llx", (unsigned long long)(uint32_t)(-disp));
                } else {
                    written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand),
                                       "+0x%llx", (unsigned long long)(uint32_t)disp);
                }
                if (written < 0 || (size_t)written >= sizeof(operand) - strlen(operand)) {
                    return -1;
                }
            }
        }

        written = snprintf(operand + strlen(operand), sizeof(operand) - strlen(operand), "]");
        if (written < 0 || (size_t)written >= sizeof(operand) - strlen(operand)) {
            return -1;
        }

        if (snprintf(out, out_size, "%s", operand) < 0) {
            return -1;
        }
        *rm_len = (int)(cursor - modrm_pos);
        return 0;
    }
}

static int decode_one(dis_ctx_t *ctx, dis_decoded_t *out) {
    size_t start;
    size_t cursor;
    int operand_override = 0;
    uint8_t rex = 0;
    int rex_w = 0;
    int rex_r = 0;
    int rex_b = 0;
    uint8_t opcode;

    if (!ctx || !out) {
        return -1;
    }
    if (ctx->offset >= ctx->code_size) {
        return -1;
    }

    memset(out, 0, sizeof(*out));

    start = ctx->offset;
    cursor = start;

    while (cursor < ctx->code_size) {
        uint8_t b = ctx->code[cursor];
        if (b == 0x66) {
            operand_override = 1;
            cursor++;
            continue;
        }
        if (b == 0x67 || b == 0xf0 || b == 0xf2 || b == 0xf3 ||
            b == 0x2e || b == 0x36 || b == 0x3e || b == 0x26 ||
            b == 0x64 || b == 0x65) {
            cursor++;
            continue;
        }
        if (b >= 0x40 && b <= 0x4f) {
            rex = b;
            cursor++;
            continue;
        }
        break;
    }

    if (cursor >= ctx->code_size) {
        return -1;
    }

    rex_w = (rex >> 3) & 0x1;
    rex_r = (rex >> 2) & 0x1;
    rex_b = rex & 0x1;

    opcode = ctx->code[cursor++];

    if (opcode >= 0x50 && opcode <= 0x57) {
        int reg = (opcode - 0x50) + (rex_b << 3);
        snprintf(out->text, sizeof(out->text), "push %s", reg64_name(reg));
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode >= 0x58 && opcode <= 0x5f) {
        int reg = (opcode - 0x58) + (rex_b << 3);
        snprintf(out->text, sizeof(out->text), "pop %s", reg64_name(reg));
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0x6a) {
        if (cursor >= ctx->code_size) {
            return -1;
        }
        {
            int8_t imm8 = (int8_t)ctx->code[cursor++];
            if (imm8 < 0) {
                snprintf(out->text, sizeof(out->text), "push -0x%llx",
                         (unsigned long long)(uint8_t)(-imm8));
            } else {
                snprintf(out->text, sizeof(out->text), "push 0x%llx",
                         (unsigned long long)(uint8_t)imm8);
            }
        }
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0x68) {
        uint32_t imm32;
        if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
            return -1;
        }
        cursor += 4U;
        snprintf(out->text, sizeof(out->text), "push 0x%llx", (unsigned long long)imm32);
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode >= 0x70 && opcode <= 0x7f) {
        int8_t rel8;
        uint64_t target;
        static const char *jcc_names[16] = {
            "jo", "jno", "jb", "jae", "je", "jne", "jbe", "ja",
            "js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"
        };

        if (cursor >= ctx->code_size) {
            return -1;
        }

        rel8 = (int8_t)ctx->code[cursor++];
        {
            int64_t next_ip = (int64_t)ctx->base_address + (int64_t)start + (int64_t)(cursor - start);
            target = (uint64_t)(next_ip + (int64_t)rel8);
        }
        snprintf(out->text, sizeof(out->text), "%s 0x%llx",
                 jcc_names[opcode - 0x70],
                 (unsigned long long)target);
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0xeb) {
        int8_t rel8;
        uint64_t target;

        if (cursor >= ctx->code_size) {
            return -1;
        }

        rel8 = (int8_t)ctx->code[cursor++];
        {
            int64_t next_ip = (int64_t)ctx->base_address + (int64_t)start + (int64_t)(cursor - start);
            target = (uint64_t)(next_ip + (int64_t)rel8);
        }
        snprintf(out->text, sizeof(out->text), "jmp 0x%llx", (unsigned long long)target);
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0x90) {
        snprintf(out->text, sizeof(out->text), "nop");
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0xc9) {
        snprintf(out->text, sizeof(out->text), "leave");
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0x0f) {
        if (cursor >= ctx->code_size) {
            return -1;
        }

        {
            uint8_t op2 = ctx->code[cursor++];
            if (op2 == 0x05) {
                snprintf(out->text, sizeof(out->text), "syscall");
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0x1f) {
                uint8_t modrm;
                int ext;
                int rm_len;
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                ext = (modrm >> 3) & 0x7;
                if (ext != 0) {
                    snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                    out->consumed = 1;
                    out->byte_count = 1;
                    out->bytes[0] = ctx->code[start];
                    return 0;
                }

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, 32,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                if (((modrm >> 6) & 0x3) == 0x3) {
                    snprintf(out->text, sizeof(out->text), "nop");
                } else {
                    snprintf(out->text, sizeof(out->text), "nop %s", rm_text);
                }
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 >= 0x80 && op2 <= 0x8f) {
                uint32_t rel32;
                int32_t rel;
                uint64_t target;
                static const char *jcc_names[16] = {
                    "jo", "jno", "jb", "jae", "je", "jne", "jbe", "ja",
                    "js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"
                };

                if (read_le32(ctx->code, ctx->code_size, cursor, &rel32) < 0) {
                    return -1;
                }
                rel = (int32_t)rel32;
                cursor += 4U;
                {
                    int64_t next_ip = (int64_t)ctx->base_address + (int64_t)start + (int64_t)(cursor - start);
                    target = (uint64_t)(next_ip + (int64_t)rel);
                }
                snprintf(out->text, sizeof(out->text), "%s 0x%llx",
                         jcc_names[op2 - 0x80],
                         (unsigned long long)target);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 >= 0x40 && op2 <= 0x4f) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                char rm_text[128];
                int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                static const char *cmov_names[16] = {
                    "cmovo", "cmovno", "cmovb", "cmovae", "cmove", "cmovne", "cmovbe", "cmova",
                    "cmovs", "cmovns", "cmovp", "cmovnp", "cmovl", "cmovge", "cmovle", "cmovg"
                };

                if (cursor >= ctx->code_size) {
                    return -1;
                }
                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "%s %s, %s",
                         cmov_names[op2 - 0x40],
                         select_reg_name(reg_field, size_bits, rex != 0),
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 >= 0x90 && op2 <= 0x9f) {
                int rm_len;
                char rm_text[128];
                static const char *set_names[16] = {
                    "seto", "setno", "setb", "setae", "sete", "setne", "setbe", "seta",
                    "sets", "setns", "setp", "setnp", "setl", "setge", "setle", "setg"
                };

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, 8,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "%s %s",
                         set_names[op2 - 0x90],
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xb6 || op2 == 0xb7) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int src_size_bits = (op2 == 0xb6) ? 8 : 16;
                int dst_size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, src_size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "movzx %s, %s",
                         select_reg_name(reg_field, dst_size_bits, rex != 0),
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xbe || op2 == 0xbf) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int src_size_bits = (op2 == 0xbe) ? 8 : 16;
                int dst_size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, src_size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "movsx %s, %s",
                         select_reg_name(reg_field, dst_size_bits, rex != 0),
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xaf) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "imul %s, %s",
                         select_reg_name(reg_field, size_bits, rex != 0),
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xbc || op2 == 0xbd) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                char rm_text[128];
                const char *mnemonic = (op2 == 0xbc) ? "bsf" : "bsr";

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "%s %s, %s",
                         mnemonic,
                         select_reg_name(reg_field, size_bits, rex != 0),
                         rm_text);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xc0 || op2 == 0xc1) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int size_bits = (op2 == 0xc0) ? 8 : (rex_w ? 64 : (operand_override ? 16 : 32));
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "xadd %s, %s",
                         rm_text,
                         select_reg_name(reg_field, size_bits, rex != 0));
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xb0 || op2 == 0xb1) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int size_bits = (op2 == 0xb0) ? 8 : (rex_w ? 64 : (operand_override ? 16 : 32));
                char rm_text[128];

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "cmpxchg %s, %s",
                         rm_text,
                         select_reg_name(reg_field, size_bits, rex != 0));
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xa3 || op2 == 0xab || op2 == 0xb3 || op2 == 0xbb) {
                uint8_t modrm;
                int reg_field;
                int rm_len;
                int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                char rm_text[128];
                const char *mnemonic = "bt";

                if (op2 == 0xab) {
                    mnemonic = "bts";
                } else if (op2 == 0xb3) {
                    mnemonic = "btr";
                } else if (op2 == 0xbb) {
                    mnemonic = "btc";
                }

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                snprintf(out->text, sizeof(out->text), "%s %s, %s",
                         mnemonic,
                         rm_text,
                         select_reg_name(reg_field, size_bits, rex != 0));
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
            if (op2 == 0xba) {
                uint8_t modrm;
                int ext;
                int rm_len;
                int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
                uint8_t imm8;
                char rm_text[128];
                const char *mnemonic = NULL;

                if (cursor >= ctx->code_size) {
                    return -1;
                }

                modrm = ctx->code[cursor];
                ext = (modrm >> 3) & 0x7;

                if (ext == 4) {
                    mnemonic = "bt";
                } else if (ext == 5) {
                    mnemonic = "bts";
                } else if (ext == 6) {
                    mnemonic = "btr";
                } else if (ext == 7) {
                    mnemonic = "btc";
                } else {
                    snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                    out->consumed = 1;
                    out->byte_count = 1;
                    out->bytes[0] = ctx->code[start];
                    return 0;
                }

                if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, size_bits,
                                      rm_text, sizeof(rm_text), &rm_len) < 0) {
                    return -1;
                }
                cursor += (size_t)rm_len;

                if (cursor >= ctx->code_size) {
                    return -1;
                }
                imm8 = ctx->code[cursor++];

                snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                         mnemonic,
                         rm_text,
                         (unsigned long long)imm8);
                out->consumed = (int)(cursor - start);
                out->byte_count = out->consumed;
                memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
                return 0;
            }
        }

        snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
        out->consumed = 1;
        out->byte_count = 1;
        out->bytes[0] = ctx->code[start];
        return 0;
    }

    if (opcode >= 0xb8 && opcode <= 0xbf) {
        int reg = (opcode - 0xb8) + (rex_b << 3);
        int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);

        if (rex_w) {
            uint64_t imm64;
            if (read_le64(ctx->code, ctx->code_size, cursor, &imm64) < 0) {
                return -1;
            }
            cursor += 8U;
            snprintf(out->text, sizeof(out->text), "mov %s, 0x%llx",
                     select_reg_name(reg, size_bits, rex != 0),
                     (unsigned long long)imm64);
        } else if (operand_override) {
            uint16_t imm16;
            if (cursor + 2U > ctx->code_size) {
                return -1;
            }
            imm16 = (uint16_t)ctx->code[cursor] |
                    ((uint16_t)ctx->code[cursor + 1U] << 8);
            cursor += 2U;
            snprintf(out->text, sizeof(out->text), "mov %s, 0x%llx",
                     select_reg_name(reg, size_bits, rex != 0),
                     (unsigned long long)imm16);
        } else {
            uint32_t imm32;
            if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
                return -1;
            }
            cursor += 4U;
            snprintf(out->text, sizeof(out->text), "mov %s, 0x%llx",
                     select_reg_name(reg, size_bits, rex != 0),
                     (unsigned long long)imm32);
        }

        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0xe8 || opcode == 0xe9) {
        uint32_t rel32;
        int32_t rel;
        uint64_t target;
        const char *mn = (opcode == 0xe8) ? "call" : "jmp";

        if (read_le32(ctx->code, ctx->code_size, cursor, &rel32) < 0) {
            return -1;
        }

        rel = (int32_t)rel32;
        cursor += 4U;
        {
            int64_t next_ip = (int64_t)ctx->base_address + (int64_t)start + (int64_t)(cursor - start);
            target = (uint64_t)(next_ip + (int64_t)rel);
        }
        snprintf(out->text, sizeof(out->text), "%s 0x%llx", mn, (unsigned long long)target);

        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0xc3) {
        snprintf(out->text, sizeof(out->text), "ret");
        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    if (opcode == 0x80 || opcode == 0x88 || opcode == 0x8a || opcode == 0xc0 || opcode == 0xc1 ||
        opcode == 0xc7 || opcode == 0x81 || opcode == 0x83 || opcode == 0xff || opcode == 0xf6 || opcode == 0xf7 || opcode == 0x8f ||
        opcode == 0x8d || opcode == 0x63 ||
        opcode == 0x89 || opcode == 0x8b || opcode == 0x85 || opcode == 0x01 || opcode == 0x03 ||
        opcode == 0x29 || opcode == 0x2b || opcode == 0x39 || opcode == 0x3b ||
        opcode == 0x31 || opcode == 0x33 || opcode == 0x21 || opcode == 0x23 ||
        opcode == 0x09 || opcode == 0x0b) {
        uint8_t modrm;
        int mod;
        int reg_field;
        int rm_field;
        int rm_len;
        char rm_text[128];
        int size_bits = rex_w ? 64 : (operand_override ? 16 : 32);
        int rm_size_bits = size_bits;

        if (opcode == 0x63) {
            rm_size_bits = 32;
        } else if (opcode == 0x80) {
            rm_size_bits = 8;
        } else if (opcode == 0x88 || opcode == 0x8a || opcode == 0xc0 || opcode == 0xf6) {
            rm_size_bits = 8;
        }

        if (cursor >= ctx->code_size) {
            return -1;
        }

        modrm = ctx->code[cursor];
        mod = (modrm >> 6) & 0x3;
        reg_field = ((modrm >> 3) & 0x7) + (rex_r << 3);
        rm_field = (modrm & 0x7) + (rex_b << 3);

        if (format_rm_operand(ctx->code, ctx->code_size, cursor, rex, rm_size_bits,
                              rm_text, sizeof(rm_text), &rm_len) < 0) {
            return -1;
        }
        cursor += (size_t)rm_len;

        if (opcode == 0x8d) {
            snprintf(out->text, sizeof(out->text), "lea %s, %s",
                     select_reg_name(reg_field, size_bits, rex != 0), rm_text);
        } else if (opcode == 0x63) {
            snprintf(out->text, sizeof(out->text), "movsxd %s, %s",
                     reg64_name(reg_field), rm_text);
        } else if (opcode == 0x89) {
            snprintf(out->text, sizeof(out->text), "mov %s, %s",
                     rm_text, select_reg_name(reg_field, size_bits, rex != 0));
        } else if (opcode == 0x8b) {
            snprintf(out->text, sizeof(out->text), "mov %s, %s",
                     select_reg_name(reg_field, size_bits, rex != 0), rm_text);
        } else if (opcode == 0x88) {
            snprintf(out->text, sizeof(out->text), "mov %s, %s",
                     rm_text, select_reg_name(reg_field, 8, rex != 0));
        } else if (opcode == 0x8a) {
            snprintf(out->text, sizeof(out->text), "mov %s, %s",
                     select_reg_name(reg_field, 8, rex != 0), rm_text);
        } else if (opcode == 0x85) {
            snprintf(out->text, sizeof(out->text), "test %s, %s",
                     rm_text, select_reg_name(reg_field, size_bits, rex != 0));
        } else if (opcode == 0xc0 || opcode == 0xc1) {
            const char *mnemonic = NULL;
            int ext = (modrm >> 3) & 0x7;

            if (x86_lookup_group2_mnemonic((uint8_t)ext, &mnemonic) < 0) {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
            if (cursor >= ctx->code_size) {
                return -1;
            }
            {
                uint8_t imm8 = ctx->code[cursor++];
                snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                         mnemonic, rm_text, (unsigned long long)imm8);
            }
        } else if (opcode == 0x01 || opcode == 0x03 || opcode == 0x29 || opcode == 0x2b ||
                   opcode == 0x39 || opcode == 0x3b || opcode == 0x31 || opcode == 0x33 ||
                   opcode == 0x21 || opcode == 0x23 || opcode == 0x09 || opcode == 0x0b) {
            const char *mnemonic = "";
            int reg_dst = (opcode == 0x03 || opcode == 0x2b || opcode == 0x3b ||
                           opcode == 0x33 || opcode == 0x23 || opcode == 0x0b);

            switch (opcode) {
                case 0x01:
                case 0x03:
                    mnemonic = "add";
                    break;
                case 0x29:
                case 0x2b:
                    mnemonic = "sub";
                    break;
                case 0x39:
                case 0x3b:
                    mnemonic = "cmp";
                    break;
                case 0x31:
                case 0x33:
                    mnemonic = "xor";
                    break;
                case 0x21:
                case 0x23:
                    mnemonic = "and";
                    break;
                case 0x09:
                case 0x0b:
                    mnemonic = "or";
                    break;
                default:
                    mnemonic = "db";
                    break;
            }

            if (!reg_dst) {
                snprintf(out->text, sizeof(out->text), "%s %s, %s",
                         mnemonic, rm_text, select_reg_name(reg_field, size_bits, rex != 0));
            } else {
                snprintf(out->text, sizeof(out->text), "%s %s, %s",
                         mnemonic, select_reg_name(reg_field, size_bits, rex != 0), rm_text);
            }
        } else if (opcode == 0xc7) {
            int ext = (modrm >> 3) & 0x7;
            if (ext != 0) {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
            if (operand_override) {
                uint16_t imm16;
                if (cursor + 2U > ctx->code_size) {
                    return -1;
                }
                imm16 = (uint16_t)ctx->code[cursor] |
                        ((uint16_t)ctx->code[cursor + 1U] << 8);
                cursor += 2U;
                snprintf(out->text, sizeof(out->text), "mov %s, 0x%llx",
                         rm_text, (unsigned long long)imm16);
            } else {
                uint32_t imm32;
                if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
                    return -1;
                }
                cursor += 4U;
                snprintf(out->text, sizeof(out->text), "mov %s, 0x%llx",
                         rm_text, (unsigned long long)imm32);
            }
        } else if (opcode == 0x80 || opcode == 0x81 || opcode == 0x83) {
            const char *mnemonic = NULL;
            int ext = (modrm >> 3) & 0x7;
            if (x86_lookup_group1_mnemonic((uint8_t)ext, &mnemonic) < 0) {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
            if (opcode == 0x80) {
                if (cursor >= ctx->code_size) {
                    return -1;
                }
                {
                    uint8_t imm8 = ctx->code[cursor++];
                    snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                             mnemonic, rm_text,
                             (unsigned long long)imm8);
                }
            } else if (opcode == 0x81) {
                if (operand_override) {
                    uint16_t imm16;
                    if (cursor + 2U > ctx->code_size) {
                        return -1;
                    }
                    imm16 = (uint16_t)ctx->code[cursor] |
                            ((uint16_t)ctx->code[cursor + 1U] << 8);
                    cursor += 2U;
                    snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                             mnemonic, rm_text, (unsigned long long)imm16);
                } else {
                    uint32_t imm32;
                    if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
                        return -1;
                    }
                    cursor += 4U;
                    snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                             mnemonic, rm_text, (unsigned long long)imm32);
                }
            } else {
                if (cursor >= ctx->code_size) {
                    return -1;
                }
                {
                    int8_t imm8 = (int8_t)ctx->code[cursor++];
                    if (imm8 < 0) {
                        snprintf(out->text, sizeof(out->text), "%s %s, -0x%llx",
                                 mnemonic, rm_text,
                                 (unsigned long long)(uint8_t)(-imm8));
                    } else {
                        snprintf(out->text, sizeof(out->text), "%s %s, 0x%llx",
                                 mnemonic, rm_text,
                                 (unsigned long long)(uint8_t)imm8);
                    }
                }
            }
        } else if (opcode == 0xff) {
            int ext = (modrm >> 3) & 0x7;
            if (ext == 0) {
                snprintf(out->text, sizeof(out->text), "inc %s", rm_text);
            } else if (ext == 1) {
                snprintf(out->text, sizeof(out->text), "dec %s", rm_text);
            } else if (ext == 2) {
                snprintf(out->text, sizeof(out->text), "call %s", rm_text);
            } else if (ext == 4) {
                snprintf(out->text, sizeof(out->text), "jmp %s", rm_text);
            } else if (ext == 6) {
                snprintf(out->text, sizeof(out->text), "push %s", rm_text);
            } else {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
        } else if (opcode == 0x8f) {
            int ext = (modrm >> 3) & 0x7;
            if (ext == 0) {
                snprintf(out->text, sizeof(out->text), "pop %s", rm_text);
            } else {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
        } else if (opcode == 0xf6 || opcode == 0xf7) {
            int ext = (modrm >> 3) & 0x7;
            if (ext == 0) {
                if (opcode == 0xf6) {
                    if (cursor >= ctx->code_size) {
                        return -1;
                    }
                    {
                        uint8_t imm8 = ctx->code[cursor++];
                        snprintf(out->text, sizeof(out->text), "test %s, 0x%llx",
                                 rm_text, (unsigned long long)imm8);
                    }
                } else {
                    if (rex_w) {
                        uint32_t imm32;
                        if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
                            return -1;
                        }
                        cursor += 4U;
                        snprintf(out->text, sizeof(out->text), "test %s, 0x%llx",
                                 rm_text, (unsigned long long)imm32);
                    } else if (operand_override) {
                        uint16_t imm16;
                        if (cursor + 2U > ctx->code_size) {
                            return -1;
                        }
                        imm16 = (uint16_t)ctx->code[cursor] |
                                ((uint16_t)ctx->code[cursor + 1U] << 8);
                        cursor += 2U;
                        snprintf(out->text, sizeof(out->text), "test %s, 0x%llx",
                                 rm_text, (unsigned long long)imm16);
                    } else {
                        uint32_t imm32;
                        if (read_le32(ctx->code, ctx->code_size, cursor, &imm32) < 0) {
                            return -1;
                        }
                        cursor += 4U;
                        snprintf(out->text, sizeof(out->text), "test %s, 0x%llx",
                                 rm_text, (unsigned long long)imm32);
                    }
                }
            } else if (ext == 2) {
                snprintf(out->text, sizeof(out->text), "not %s", rm_text);
            } else if (ext == 3) {
                snprintf(out->text, sizeof(out->text), "neg %s", rm_text);
            } else {
                snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
                out->consumed = 1;
                out->byte_count = 1;
                out->bytes[0] = ctx->code[start];
                return 0;
            }
        }

        if (mod == 3) {
            (void)rm_field;
        }

        out->consumed = (int)(cursor - start);
        out->byte_count = out->consumed;
        memcpy(out->bytes, &ctx->code[start], (size_t)out->byte_count);
        return 0;
    }

    snprintf(out->text, sizeof(out->text), "db 0x%02x", ctx->code[start]);
    out->consumed = 1;
    out->byte_count = 1;
    out->bytes[0] = ctx->code[start];
    return 0;
}

int disassemble_code_buffer(const uint8_t *code, size_t code_size,
                            uint64_t base_address, FILE *out) {
    dis_ctx_t ctx;

    if (!code || !out) {
        return -1;
    }

    ctx.code = code;
    ctx.code_size = code_size;
    ctx.base_address = base_address;
    ctx.offset = 0;

    while (ctx.offset < ctx.code_size) {
        dis_decoded_t inst;
        char bytes_buf[3 * DIS_MAX_INST_BYTES + 1];
        size_t pos = 0;

        if (decode_one(&ctx, &inst) < 0 || inst.consumed <= 0) {
            fprintf(out, "%016llx: %02x                                          db 0x%02x\n",
                    (unsigned long long)(ctx.base_address + ctx.offset),
                    (unsigned int)ctx.code[ctx.offset],
                    (unsigned int)ctx.code[ctx.offset]);
            ctx.offset++;
            continue;
        }

        bytes_buf[0] = '\0';
        for (int i = 0; i < inst.byte_count; i++) {
            int written;
            written = snprintf(&bytes_buf[pos], sizeof(bytes_buf) - pos,
                               (i == 0) ? "%02x" : " %02x",
                               inst.bytes[i]);
            if (written < 0 || (size_t)written >= sizeof(bytes_buf) - pos) {
                return -1;
            }
            pos += (size_t)written;
        }

        fprintf(out, "%016llx: %-43s %s\n",
                (unsigned long long)(ctx.base_address + ctx.offset),
                bytes_buf,
                inst.text);

        ctx.offset += (size_t)inst.consumed;
    }

    return 0;
}

int asm_disassemble_file(const char *filename, FILE *out) {
    dis_elf64_ehdr_t ehdr;
    dis_elf64_shdr_t *sections = NULL;
    char *shstrtab = NULL;
    uint8_t *file_data = NULL;
    FILE *fp = NULL;
    long file_size_long;
    size_t file_size;
    int result = -1;

    if (!filename || !out) {
        fprintf(stderr,
                "Error at line 1, column 1: [CLI] Missing disassembler input\n"
                "Suggestion: Provide an ELF64 binary path for disassembly.\n");
        return -1;
    }

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to open input '%s'\n"
                "Suggestion: Verify file path and read permissions.\n",
                filename);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to seek input '%s'\n"
                "Suggestion: Ensure the file is readable and not truncated.\n",
                filename);
        goto cleanup;
    }

    file_size_long = ftell(fp);
    if (file_size_long < 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to determine file size for '%s'\n"
                "Suggestion: Ensure the input is a regular file.\n",
                filename);
        goto cleanup;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to rewind input '%s'\n"
                "Suggestion: Retry with a local file path.\n",
                filename);
        goto cleanup;
    }

    file_size = (size_t)file_size_long;
    if (file_size < sizeof(ehdr)) {
        fprintf(stderr,
                "Error at line 1, column 1: [Disassembler] Input '%s' is too small for ELF64\n"
                "Suggestion: Provide a valid ELF64 executable or object file.\n",
                filename);
        goto cleanup;
    }

    file_data = (uint8_t *)malloc(file_size);
    if (!file_data) {
        fprintf(stderr,
                "Error at line 1, column 1: [Memory] Failed to allocate disassembler buffer\n"
                "Suggestion: Retry and verify memory availability.\n");
        goto cleanup;
    }

    if (fread(file_data, 1, file_size, fp) != file_size) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to read input '%s'\n"
                "Suggestion: Ensure the file is not being modified concurrently.\n",
                filename);
        goto cleanup;
    }

    memcpy(&ehdr, file_data, sizeof(ehdr));

    if (ehdr.e_ident[0] != ELFMAG0 || ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 || ehdr.e_ident[3] != ELFMAG3 ||
        ehdr.e_ident[4] != ELFCLASS64 || ehdr.e_ident[5] != ELFDATA2LSB) {
        fprintf(stderr,
                "Error at line 1, column 1: [Disassembler] Input '%s' is not ELF64 little-endian\n"
                "Suggestion: Use a 64-bit little-endian ELF binary.\n",
                filename);
        goto cleanup;
    }

    if (ehdr.e_shentsize != sizeof(dis_elf64_shdr_t) || ehdr.e_shnum == 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [Disassembler] Invalid section header table in '%s'\n"
                "Suggestion: Rebuild the binary and retry disassembly.\n",
                filename);
        goto cleanup;
    }

    if (ehdr.e_shoff + (uint64_t)ehdr.e_shentsize * ehdr.e_shnum > file_size) {
        fprintf(stderr,
                "Error at line 1, column 1: [Disassembler] Section headers are out of file bounds in '%s'\n"
                "Suggestion: Verify the ELF file integrity.\n",
                filename);
        goto cleanup;
    }

    sections = (dis_elf64_shdr_t *)malloc((size_t)ehdr.e_shentsize * ehdr.e_shnum);
    if (!sections) {
        fprintf(stderr,
                "Error at line 1, column 1: [Memory] Failed to allocate section table\n"
                "Suggestion: Retry and verify memory availability.\n");
        goto cleanup;
    }

    memcpy(sections, file_data + ehdr.e_shoff, (size_t)ehdr.e_shentsize * ehdr.e_shnum);

    if (ehdr.e_shstrndx >= ehdr.e_shnum) {
        fprintf(stderr,
                "Error at line 1, column 1: [Disassembler] Invalid section string table index in '%s'\n"
                "Suggestion: Verify ELF section metadata.\n",
                filename);
        goto cleanup;
    }

    {
        dis_elf64_shdr_t *shstr = &sections[ehdr.e_shstrndx];
        if (shstr->sh_offset + shstr->sh_size > file_size || shstr->sh_size == 0) {
            fprintf(stderr,
                    "Error at line 1, column 1: [Disassembler] Invalid section-name table bounds in '%s'\n"
                    "Suggestion: Verify ELF section headers.\n",
                    filename);
            goto cleanup;
        }

        shstrtab = (char *)malloc((size_t)shstr->sh_size);
        if (!shstrtab) {
            fprintf(stderr,
                    "Error at line 1, column 1: [Memory] Failed to allocate section-name table\n"
                    "Suggestion: Retry and verify memory availability.\n");
            goto cleanup;
        }

        memcpy(shstrtab, file_data + shstr->sh_offset, (size_t)shstr->sh_size);
    }

    {
        const uint8_t *text_data = NULL;
        size_t text_size = 0;
        uint64_t text_addr = 0;

        for (uint16_t i = 0; i < ehdr.e_shnum; i++) {
            dis_elf64_shdr_t *sh = &sections[i];
            const char *name;

            if (sh->sh_name >= sections[ehdr.e_shstrndx].sh_size) {
                continue;
            }

            name = shstrtab + sh->sh_name;
            if (strcmp(name, ".text") == 0) {
                if (sh->sh_offset + sh->sh_size > file_size) {
                    fprintf(stderr,
                            "Error at line 1, column 1: [Disassembler] .text section exceeds file size in '%s'\n"
                            "Suggestion: Verify ELF section offsets and sizes.\n",
                            filename);
                    goto cleanup;
                }
                text_data = file_data + sh->sh_offset;
                text_size = (size_t)sh->sh_size;
                text_addr = sh->sh_addr;
                break;
            }
        }

        if (!text_data) {
            fprintf(stderr,
                    "Error at line 1, column 1: [Disassembler] Missing .text section in '%s'\n"
                    "Suggestion: Provide an ELF binary with executable code section.\n",
                    filename);
            goto cleanup;
        }

        if (disassemble_code_buffer(text_data, text_size, text_addr, out) < 0) {
            fprintf(stderr,
                    "Error at line 1, column 1: [Disassembler] Failed to decode .text in '%s'\n"
                    "Suggestion: Retry with a binary produced by this assembler for baseline support.\n",
                    filename);
            goto cleanup;
        }
    }

    result = 0;

cleanup:
    free(shstrtab);
    free(sections);
    free(file_data);
    if (fp) {
        fclose(fp);
    }
    return result;
}
