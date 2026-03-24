/**
 * x86_64 Assembler - Main assembler core
 * Symbol table, fixup resolution, and ELF64 output
 */

#include "x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* External functions */
extern parsed_instruction_t *parse_source(const char *source, int *count);
extern parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count);
extern void free_instructions(parsed_instruction_t *insts);

/* Encoder functions */
extern int encode_mov(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_push_pop(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_unary_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_lea(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_jmp(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_jcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_call_ret(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_int(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_syscall(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_sysret(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_nop(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_hlt(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cwd_cdq_cqo(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cbw_cwde_cdqe(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_clc_stc_cmc(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cld_std(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cli_sti(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_leave(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_enter(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_setcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_cmov(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bswap(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_xchg(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_imul(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_div_idiv_mul(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_test(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_shift(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_string(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bit_manip(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_shld_shrd(assembler_context_t *ctx, const parsed_instruction_t *inst);
extern int encode_bit_scan(assembler_context_t *ctx, const parsed_instruction_t *inst);

extern int emit_byte(assembler_context_t *ctx, uint8_t byte);

/* ============================================================================
 * HASH TABLE FOR SYMBOL LOOKUP
 * ============================================================================ */

/* FNV-1a hash function for strings */
static uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = 2166136261U;  /* FNV offset basis */
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619U;  /* FNV prime */
    }
    return hash;
}

/* Lookup symbol in hash table - O(1) average case */
hash_entry_t *symbol_hash_lookup(assembler_context_t *ctx, const char *name) {
    uint32_t hash = fnv1a_hash(name);
    int idx = hash % HASH_TABLE_SIZE;
    
    hash_entry_t *entry = ctx->symbol_hash[idx];
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Insert symbol into hash table */
int symbol_hash_insert(assembler_context_t *ctx, const char *name, uint64_t address,
                               bool is_global, bool is_external, int section) {
    uint32_t hash = fnv1a_hash(name);
    int idx = hash % HASH_TABLE_SIZE;
    
    /* Create new entry */
    hash_entry_t *entry = malloc(sizeof(hash_entry_t));
    if (!entry) {
        fprintf(stderr, "Error: Out of memory for symbol hash table\n");
        return -1;
    }
    
    strncpy(entry->name, name, MAX_LABEL_LENGTH - 1);
    entry->name[MAX_LABEL_LENGTH - 1] = '\0';
    entry->address = address;
    entry->is_resolved = true;
    entry->is_global = is_global;
    entry->is_external = is_external;
    entry->section = section;
    entry->next = ctx->symbol_hash[idx];
    ctx->symbol_hash[idx] = entry;
    
    return 0;
}

/* Update existing symbol in hash table */
int symbol_hash_update(assembler_context_t *ctx, const char *name, uint64_t address,
                               bool is_global, bool is_external, int section) {
    hash_entry_t *entry = symbol_hash_lookup(ctx, name);
    if (entry) {
        entry->address = address;
        entry->is_resolved = true;
        entry->is_global = is_global;
        entry->is_external = is_external;
        entry->section = section;
        return 0;
    }
    return -1;  /* Not found */
}

/* ============================================================================
 * CONTEXT MANAGEMENT
 * ============================================================================ */

assembler_context_t *asm_init(void) {
    assembler_context_t *ctx = calloc(1, sizeof(assembler_context_t));
    if (!ctx) return NULL;

    ctx->text_capacity = MAX_OUTPUT_SIZE;
    ctx->text_section = malloc(ctx->text_capacity);
    if (!ctx->text_section) {
        free(ctx);
        return NULL;
    }

    ctx->data_capacity = MAX_OUTPUT_SIZE;
    ctx->data_section = malloc(ctx->data_capacity);
    if (!ctx->data_section) {
        free(ctx->text_section);
        free(ctx);
        return NULL;
    }

    ctx->base_address = 0x400000; /* Default Linux base address */
    ctx->current_section = 0; /* text section */
    ctx->symbol_count = 0;
    ctx->fixup_count = 0;

    return ctx;
}

void asm_free(assembler_context_t *ctx) {
    if (!ctx) return;
    
    /* Free hash table entries */
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        hash_entry_t *entry = ctx->symbol_hash[i];
        while (entry) {
            hash_entry_t *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    free(ctx->text_section);
    free(ctx->data_section);
    free(ctx->output);
    free(ctx);
}

/* ============================================================================
 * SYMBOL TABLE
 * ============================================================================ */

int add_symbol(assembler_context_t *ctx, const char *name, uint64_t address,
               bool is_global, bool is_external, int section) {
    if (ctx->symbol_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Symbol table full\n");
        return -1;
    }

    /* Check if symbol already exists using hash table - O(1) lookup */
    hash_entry_t *existing = symbol_hash_lookup(ctx, name);
    if (existing) {
        if (!existing->is_resolved) {
            /* Update existing unresolved symbol */
            existing->address = address;
            existing->is_resolved = true;
            existing->is_global = is_global;
            existing->is_external = is_external;
            existing->section = section;
            
            /* Also update in array for consistency */
            for (int i = 0; i < ctx->symbol_count; i++) {
                if (strcmp(ctx->symbols[i].name, name) == 0) {
                    ctx->symbols[i].address = address;
                    ctx->symbols[i].is_resolved = true;
                    break;
                }
            }
            return 0;
        }
        fprintf(stderr, "Error: Redefined symbol '%s'\n", name);
        return -1;
    }

    /* Add to array (for iteration) */
    symbol_t *sym = &ctx->symbols[ctx->symbol_count++];
    strncpy(sym->name, name, MAX_LABEL_LENGTH - 1);
    sym->name[MAX_LABEL_LENGTH - 1] = '\0';
    sym->address = address;
    sym->decl_line = 0;
    sym->decl_column = 1;
    sym->is_resolved = true;
    sym->is_global = is_global;
    sym->is_external = is_external;
    sym->is_function = false;
    sym->section = section;

    /* Add to hash table (for O(1) lookup) */
    if (symbol_hash_insert(ctx, name, address, is_global, is_external, section) < 0) {
        ctx->symbol_count--;  /* Rollback */
        return -1;
    }

    return 0;
}

int find_symbol_address(assembler_context_t *ctx, const char *name, uint64_t *addr) {
    /* Use hash table for O(1) lookup instead of linear search */
    hash_entry_t *entry = symbol_hash_lookup(ctx, name);
    if (entry && entry->is_resolved) {
        *addr = entry->address;
        return 0;
    }
    return -1;
}

/* Check if symbol exists (for undefined symbol checking) */
bool symbol_exists(assembler_context_t *ctx, const char *name) {
    return symbol_hash_lookup(ctx, name) != NULL;
}

/* Get full symbol info from hash table */
hash_entry_t *get_symbol_info(assembler_context_t *ctx, const char *name) {
    return symbol_hash_lookup(ctx, name);
}

/* ============================================================================
 * FIXUP RESOLUTION
 * ============================================================================ */

int add_fixup(assembler_context_t *ctx, const char *label, uint64_t location,
              int size, instruction_type_t inst_type) {
    if (ctx->fixup_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Fixup list full\n");
        return -1;
    }

    strncpy(ctx->fixups[ctx->fixup_count].label, label, MAX_LABEL_LENGTH - 1);
    ctx->fixups[ctx->fixup_count].label[MAX_LABEL_LENGTH - 1] = '\0';
    ctx->fixups[ctx->fixup_count].location = location;
    ctx->fixups[ctx->fixup_count].size = size;
    ctx->fixups[ctx->fixup_count].instruction_type = inst_type;
    ctx->fixups[ctx->fixup_count].is_rip_relative = false;
    ctx->fixups[ctx->fixup_count].section = 0;

    ctx->fixup_count++;
    return 0;
}

int add_fixup_rip_relative(assembler_context_t *ctx, const char *label,
                           uint64_t location, int section) {
    if (ctx->fixup_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Fixup list full\n");
        return -1;
    }

    strncpy(ctx->fixups[ctx->fixup_count].label, label, MAX_LABEL_LENGTH - 1);
    ctx->fixups[ctx->fixup_count].label[MAX_LABEL_LENGTH - 1] = '\0';
    ctx->fixups[ctx->fixup_count].location = location;
    ctx->fixups[ctx->fixup_count].size = 4;  /* 32-bit displacement */
    ctx->fixups[ctx->fixup_count].instruction_type = INST_MOV;  /* Generic */
    ctx->fixups[ctx->fixup_count].is_rip_relative = true;
    ctx->fixups[ctx->fixup_count].section = section;

    ctx->fixup_count++;
    return 0;
}

int resolve_fixups(assembler_context_t *ctx) {
    for (int i = 0; i < ctx->fixup_count; i++) {
        uint64_t target;
        if (find_symbol_address(ctx, ctx->fixups[i].label, &target) < 0) {
            fprintf(stderr, "Error: Undefined symbol '%s'\n", ctx->fixups[i].label);
            return -1;
        }

        uint64_t location = ctx->fixups[i].location;
        int size = ctx->fixups[i].size;

        /* Calculate relative offset for control flow instructions */
        int64_t value;
        instruction_type_t inst_type = ctx->fixups[i].instruction_type;

        if (ctx->fixups[i].is_rip_relative) {
            /* RIP-relative addressing: offset from end of instruction to target */
            /* location points to the displacement field, size is 4 bytes */
            value = (int64_t)target - ((int64_t)location + size);
        } else if (inst_type == INST_JMP || inst_type == INST_CALL ||
            (inst_type >= INST_JA && inst_type <= INST_JS)) {
            /* Relative offset for control flow */
            value = (int64_t)target - ((int64_t)location + size);
        } else {
            /* Absolute address */
            value = (int64_t)target;
        }

        /* Write the value to the output */
        uint64_t offset = location - ctx->base_address;
        if (offset + size > ctx->text_size) {
            fprintf(stderr, "Error: Fixup location out of bounds\n");
            return -1;
        }

        /* Patch the bytes */
        if (size == 1) {
            if (value < -128 || value > 127) {
                fprintf(stderr, "Error: Fixup value out of 8-bit range\n");
                return -1;
            }
            ctx->text_section[offset] = (uint8_t)(int8_t)value;
        } else if (size == 4) {
            if (value < INT32_MIN || value > INT32_MAX) {
                fprintf(stderr, "Error: Fixup value out of 32-bit range\n");
                return -1;
            }
            int32_t v32 = (int32_t)value;
            ctx->text_section[offset] = v32 & 0xFF;
            ctx->text_section[offset + 1] = (v32 >> 8) & 0xFF;
            ctx->text_section[offset + 2] = (v32 >> 16) & 0xFF;
            ctx->text_section[offset + 3] = (v32 >> 24) & 0xFF;
        } else if (size == 8) {
            uint64_t v64 = (uint64_t)value;
            for (int j = 0; j < 8; j++) {
                ctx->text_section[offset + j] = (v64 >> (j * 8)) & 0xFF;
            }
        }
    }

    return 0;
}

/* ============================================================================
 * INSTRUCTION DISPATCH
 * ============================================================================ */

int encode_instruction(assembler_context_t *ctx, const parsed_instruction_t *inst) {
    switch (inst->type) {
        case INST_MOV:
        case INST_MOVZX:
        case INST_MOVSX:
            return encode_mov(ctx, inst);

        case INST_PUSH:
        case INST_POP:
            return encode_push_pop(ctx, inst);

        case INST_ADD:
        case INST_ADC:
        case INST_SUB:
        case INST_SBB:
        case INST_XOR:
        case INST_OR:
        case INST_AND:
        case INST_CMP:
            return encode_arithmetic(ctx, inst);

        case INST_INC:
        case INST_DEC:
        case INST_NEG:
        case INST_NOT:
            return encode_unary_arithmetic(ctx, inst);

        case INST_LEA:
            return encode_lea(ctx, inst);

        case INST_JMP:
            return encode_jmp(ctx, inst);

        case INST_JA: case INST_JAE: case INST_JB: case INST_JBE:
        case INST_JE: case INST_JG: case INST_JGE: case INST_JL:
        case INST_JLE: case INST_JNE: case INST_JNZ: case INST_JNO:
        case INST_JNP: case INST_JNS: case INST_JO: case INST_JP:
        case INST_JS:
            return encode_jcc(ctx, inst);

        case INST_CALL:
        case INST_RET:
            return encode_call_ret(ctx, inst);

        case INST_INT:
            return encode_int(ctx, inst);

        case INST_SYSCALL:
            return encode_syscall(ctx, inst);

        case INST_SYSRET:
            return encode_sysret(ctx, inst);

        case INST_NOP:
            return encode_nop(ctx, inst);

        case INST_HLT:
            return encode_hlt(ctx, inst);

        case INST_CWD:
        case INST_CQO:
        case INST_CDQ:
            return encode_cwd_cdq_cqo(ctx, inst);

        case INST_CBW:
        case INST_CWDE:
        case INST_CDQE:
            return encode_cbw_cwde_cdqe(ctx, inst);

        case INST_CLC:
        case INST_STC:
        case INST_CMC:
            return encode_clc_stc_cmc(ctx, inst);

        case INST_CLD:
        case INST_STD:
            return encode_cld_std(ctx, inst);

        case INST_CLI:
        case INST_STI:
            return encode_cli_sti(ctx, inst);

        case INST_LEAVE:
            return encode_leave(ctx, inst);

        case INST_ENTER:
            return encode_enter(ctx, inst);

        case INST_SETA: case INST_SETAE: case INST_SETB: case INST_SETBE:
        case INST_SETE: case INST_SETG: case INST_SETGE: case INST_SETL:
        case INST_SETLE: case INST_SETNE: case INST_SETNO: case INST_SETNP:
        case INST_SETNS: case INST_SETO: case INST_SETP: case INST_SETS:
            return encode_setcc(ctx, inst);

        case INST_CMOVA: case INST_CMOVAE: case INST_CMOVB: case INST_CMOVBE:
        case INST_CMOVE: case INST_CMOVG: case INST_CMOVGE: case INST_CMOVL:
        case INST_CMOVLE: case INST_CMOVNE: case INST_CMOVNO: case INST_CMOVNP:
        case INST_CMOVNS: case INST_CMOVO: case INST_CMOVP: case INST_CMOVS:
            return encode_cmov(ctx, inst);

        case INST_XCHG:
            return encode_xchg(ctx, inst);

        case INST_BSWAP:
            return encode_bswap(ctx, inst);

        case INST_IMUL:
            return encode_imul(ctx, inst);

        case INST_MUL:
        case INST_DIV:
        case INST_IDIV:
            return encode_div_idiv_mul(ctx, inst);

        case INST_TEST:
            return encode_test(ctx, inst);

        case INST_SHL: case INST_SHR: case INST_SAL: case INST_SAR:
        case INST_ROL: case INST_ROR: case INST_RCL: case INST_RCR:
            return encode_shift(ctx, inst);

        case INST_SHLD: case INST_SHRD:
            return encode_shld_shrd(ctx, inst);

        case INST_BT: case INST_BTS: case INST_BTR: case INST_BTC:
            return encode_bit_manip(ctx, inst);

        case INST_BSF: case INST_BSR:
            return encode_bit_scan(ctx, inst);

        case INST_MOVSB: case INST_MOVSW: case INST_MOVSD: case INST_MOVSQ:
            return encode_string(ctx, inst);

        /* SSE Move Instructions */
        case INST_MOVAPS: case INST_MOVUPS:
        case INST_MOVSS: case INST_MOVAPD:
        case INST_MOVUPD: case INST_MOVSD_XMM:
        case INST_MOVDQA: case INST_MOVDQU:
            return encode_sse_mov(ctx, inst);

        /* SSE Arithmetic Instructions */
        case INST_ADDSS: case INST_ADDPS: case INST_ADDSD: case INST_ADDPD:
        case INST_SUBSS: case INST_SUBPS: case INST_SUBSD: case INST_SUBPD:
        case INST_MULSS: case INST_MULPS: case INST_MULSD: case INST_MULPD:
        case INST_DIVSS: case INST_DIVPS: case INST_DIVSD: case INST_DIVPD:
            return encode_sse_arith(ctx, inst);

        /* SSE Packed Integer Instructions */
        case INST_PADDB: case INST_PADDW: case INST_PADDD: case INST_PADDQ:
        case INST_PSUBB: case INST_PSUBW: case INST_PSUBD: case INST_PSUBQ:
            return encode_sse_packed_arith(ctx, inst);

        case INST_PCMPEQB: case INST_PCMPEQW: case INST_PCMPEQD: case INST_PCMPEQQ:
        case INST_PCMPGTB: case INST_PCMPGTW: case INST_PCMPGTD: case INST_PCMPGTQ:
            return encode_sse_packed_cmp(ctx, inst);

        case INST_PAND: case INST_PANDNPD: case INST_POR: case INST_PXOR:
            return encode_sse_packed_logic(ctx, inst);

        case INST_DB:
        case INST_DW:
        case INST_DD:
        case INST_DQ:
            /* Data directives */
            for (int i = 0; i < inst->operand_count; i++) {
                if (inst->operands[i].type == OPERAND_IMM) {
                    if (inst->type == INST_DB) {
                        emit_byte(ctx, (uint8_t)inst->operands[i].immediate);
                    } else if (inst->type == INST_DW) {
                        uint16_t v = (uint16_t)inst->operands[i].immediate;
                        emit_byte(ctx, v & 0xFF);
                        emit_byte(ctx, (v >> 8) & 0xFF);
                    } else if (inst->type == INST_DD) {
                        uint32_t v = (uint32_t)inst->operands[i].immediate;
                        emit_byte(ctx, v & 0xFF);
                        emit_byte(ctx, (v >> 8) & 0xFF);
                        emit_byte(ctx, (v >> 16) & 0xFF);
                        emit_byte(ctx, (v >> 24) & 0xFF);
                    } else {
                        uint64_t v = (uint64_t)inst->operands[i].immediate;
                        for (int j = 0; j < 8; j++) {
                            emit_byte(ctx, (v >> (j * 8)) & 0xFF);
                        }
                    }
                } else if (inst->operands[i].type == OPERAND_STRING && inst->type == INST_DB) {
                    /* String literal for db directive - emit each character as a byte */
                    const char *str = inst->operands[i].string;
                    for (size_t j = 0; j < strlen(str); j++) {
                        emit_byte(ctx, (uint8_t)str[j]);
                    }
                }
            }
            return 0;

        case INST_RESB:
        case INST_RESW:
        case INST_RESD:
        case INST_RESQ:
            /* Reserve uninitialized space - emit zeros */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_IMM) {
                int64_t count = inst->operands[0].immediate;
                int size = (inst->type == INST_RESB) ? 1 :
                          (inst->type == INST_RESW) ? 2 :
                          (inst->type == INST_RESD) ? 4 : 8;
                for (int64_t i = 0; i < count * size; i++) {
                    emit_byte(ctx, 0);
                }
            }
            return 0;

        case INST_EQU:
            /* equ is handled in the first pass - no emission needed */
            return 0;

        case INST_TIMES:
            /* times is handled during parsing/expansion - should not reach here */
            return 0;

        case INST_GLOBAL:
            /* Mark symbols as global - handled in first pass */
            return 0;

        case INST_EXTERN:
            /* External symbols - handled in first pass */
            return 0;

        case INST_SECTION:
            /* Section directive - change current section */
            if (inst->operand_count > 0 && inst->operands[0].type == OPERAND_LABEL) {
                const char *section_name = inst->operands[0].label;
                if (strcmp(section_name, ".data") == 0 ||
                    strcmp(section_name, "data") == 0 ||
                    strcmp(section_name, ".bss") == 0 ||
                    strcmp(section_name, "bss") == 0 ||
                    strcmp(section_name, ".rodata") == 0 ||
                    strcmp(section_name, "rodata") == 0) {
                    ctx->current_section = 1;  /* Data section */
                } else {
                    ctx->current_section = 0;  /* Text section */
                }
            }
            return 0;

        case INST_ALIGN:
            /* Align directive - pad to alignment boundary */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_IMM) {
                int alignment = (int)inst->operands[0].immediate;
                if (alignment > 0 && (alignment & (alignment - 1)) == 0) {  /* Power of 2 */
                    uint64_t current_addr = ctx->current_address;
                    uint64_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
                    int padding = (int)(aligned_addr - current_addr);
                    
                    /* Emit padding: NOPs for text section, zeros for data section */
                    for (int i = 0; i < padding; i++) {
                        if (ctx->current_section == 0) {
                            emit_byte(ctx, 0x90);  /* NOP in text section */
                        } else {
                            emit_byte(ctx, 0x00);  /* Zero in data section */
                        }
                    }
                } else {
                    fprintf(stderr, "Error: align directive requires a power of 2 at line %d\n",
                            inst->line_number);
                    return -1;
                }
            } else {
                fprintf(stderr, "Error: align directive requires an immediate operand at line %d\n",
                        inst->line_number);
                return -1;
            }
            return 0;

        case INST_INCBIN:
            /* Include binary file - read and emit bytes */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_STRING) {
                const char *filename = inst->operands[0].string;
                FILE *fp = fopen(filename, "rb");
                if (!fp) {
                    fprintf(stderr, "Error: Cannot open incbin file '%s' at line %d\n",
                            filename, inst->line_number);
                    return -1;
                }
                
                /* Get file size */
                fseek(fp, 0, SEEK_END);
                long file_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                
                if (file_size < 0) {
                    fprintf(stderr, "Error: Cannot determine size of incbin file '%s' at line %d\n",
                            filename, inst->line_number);
                    fclose(fp);
                    return -1;
                }
                
                /* Read and emit file contents */
                uint8_t buffer[1024];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                    for (size_t i = 0; i < bytes_read; i++) {
                        emit_byte(ctx, buffer[i]);
                    }
                }
                
                if (ferror(fp)) {
                    fprintf(stderr, "Error: Failed to read incbin file '%s' at line %d\n",
                            filename, inst->line_number);
                    fclose(fp);
                    return -1;
                }
                
                fclose(fp);
            } else {
                fprintf(stderr, "Error: incbin directive requires a string filename at line %d\n",
                        inst->line_number);
                return -1;
            }
            return 0;

        case INST_COMM:
        case INST_LCOMM:
            /* Common symbol declaration - reserve uninitialized space in BSS */
            if (inst->operand_count >= 2 &&
                inst->operands[0].type == OPERAND_LABEL &&
                inst->operands[1].type == OPERAND_IMM) {
                const char *symbol = inst->operands[0].label;
                int64_t size = inst->operands[1].immediate;
                int64_t alignment = (inst->operand_count >= 3) ?
                                   inst->operands[2].immediate : 1;
                
                if (size <= 0) {
                    fprintf(stderr, "Error: %s size must be positive at line %d\n",
                            (inst->type == INST_COMM) ? ".comm" : ".lcomm",
                            inst->line_number);
                    return -1;
                }
                
                /* Add symbol to BSS section */
                /* For now, treat as data section address */
                uint64_t addr = ctx->current_address;
                
                /* Apply alignment if specified */
                if (alignment > 1) {
                    addr = (addr + alignment - 1) & ~(alignment - 1);
                    int padding = (int)(addr - ctx->current_address);
                    for (int i = 0; i < padding; i++) {
                        emit_byte(ctx, 0);
                    }
                }
                
                /* Add symbol */
                add_symbol(ctx, symbol, addr, (inst->type == INST_COMM), false, 1);
                
                /* Reserve space by emitting zeros */
                for (int64_t i = 0; i < size; i++) {
                    emit_byte(ctx, 0);
                }
            } else {
                fprintf(stderr, "Error: %s requires symbol name and size at line %d\n",
                        (inst->type == INST_COMM) ? ".comm" : ".lcomm",
                        inst->line_number);
                return -1;
            }
            return 0;

        default:
            fprintf(stderr, "Error: Unhandled instruction type %d at line %d\n",
                    inst->type, inst->line_number);
            return -1;
    }
}

/* ============================================================================
 * INSTRUCTION SIZE ESTIMATION
 * ============================================================================ */

static int estimate_instruction_size(const parsed_instruction_t *inst) {
    /* Most instructions are small, but we need reasonable estimates */
    switch (inst->type) {
        /* Single byte instructions */
        case INST_NOP: return 1;
        case INST_RET: return 1;
        case INST_LEAVE: return 1;
        case INST_HLT: return 1;
        case INST_CLC: case INST_STC: case INST_CMC:
        case INST_CLD: case INST_STD:
        case INST_CLI: case INST_STI:
        case INST_CWD: case INST_CDQ: case INST_CQO:
        case INST_CBW: case INST_CWDE: case INST_CDQE:
            return 2; /* Usually 1-2 bytes with prefixes */

        /* Syscall */
        case INST_INT: return 2;
        case INST_SYSCALL: return 2;
        case INST_SYSRET: return 2;

        /* Push/pop register */
        case INST_PUSH:
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_REG) {
                /* PUSH r64: 1 byte (or 2 with REX) */
                return (inst->operands[0].reg >= 8) ? 2 : 1;
            }
            return 5; /* PUSH imm32 */

        case INST_POP:
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_REG) {
                /* POP r64: 1 byte (or 2 with REX) */
                return (inst->operands[0].reg >= 8) ? 2 : 1;
            }
            return 1;

        /* Moves - can be various sizes */
        case INST_MOV:
            if (inst->operand_count == 2) {
                const operand_t *dst = &inst->operands[0];
                const operand_t *src = &inst->operands[1];

                /* mov reg, label - RIP-relative load (7 bytes: REX + opcode + modrm + disp32) */
                if (dst->type == OPERAND_REG && src->type == OPERAND_LABEL) {
                    return 7;
                }

                /* mov r64, imm64: 10 bytes max */
                if (dst->type == OPERAND_REG && src->type == OPERAND_IMM) {
                    int64_t imm = src->immediate;
                    if (imm >= INT32_MIN && imm <= INT32_MAX) {
                        /* mov r/m64, imm32: REX + opcode + modrm + 4 bytes */
                        return 8;
                    }
                    return 10; /* mov r64, imm64 */
                }

                /* mov reg, reg: 3 bytes (REX + opcode + modrm) */
                if (dst->type == OPERAND_REG && src->type == OPERAND_REG) {
                    return 3;
                }

                /* mov reg, mem or mem, reg: up to 9 bytes with SIB and displacement */
                if ((dst->type == OPERAND_REG && src->type == OPERAND_MEM) ||
                    (dst->type == OPERAND_MEM && src->type == OPERAND_REG)) {
                    return 9;
                }

                /* mov mem, imm: REX + opcode + modrm + SIB + disp + 4 bytes imm */
                if (dst->type == OPERAND_MEM && src->type == OPERAND_IMM) {
                    return 13;
                }
            }
            return 10;

        /* Arithmetic - similar to mov for reg, imm */
        case INST_ADD: case INST_SUB: case INST_AND: case INST_OR: case INST_XOR:
        case INST_CMP: case INST_ADC: case INST_SBB:
            if (inst->operand_count == 2) {
                const operand_t *dst = &inst->operands[0];
                const operand_t *src = &inst->operands[1];

                if (dst->type == OPERAND_REG && src->type == OPERAND_IMM) {
                    int64_t imm = src->immediate;
                    /* imm8 form: 4 bytes, imm32 form: 7 bytes */
                    if (imm >= -128 && imm <= 127) {
                        return 4;
                    }
                    return 7;
                }

                /* reg, reg or reg, mem: 3-9 bytes */
                if ((dst->type == OPERAND_REG && src->type == OPERAND_REG) ||
                    (dst->type == OPERAND_REG && src->type == OPERAND_MEM) ||
                    (dst->type == OPERAND_MEM && src->type == OPERAND_REG)) {
                    return 9;
                }

                /* mem, imm: up to 14 bytes */
                if (dst->type == OPERAND_MEM && src->type == OPERAND_IMM) {
                    return 14;
                }
            }
            return 10;

        /* Unary arithmetic */
        case INST_INC: case INST_DEC: case INST_NEG: case INST_NOT:
            if (inst->operand_count == 1) {
                if (inst->operands[0].type == OPERAND_REG) {
                    return 3; /* REX + opcode + modrm */
                }
                if (inst->operands[0].type == OPERAND_MEM) {
                    return 9; /* With SIB and displacement */
                }
            }
            return 4;

        /* Jumps and calls */
        case INST_JMP:
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_LABEL) {
                return 5; /* Near jump E9 cd */
            }
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_REG) {
                return 3; /* FF /4 with REX */
            }
            return 9; /* Memory indirect with SIB */

        case INST_CALL:
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_LABEL) {
                return 5; /* E8 cd */
            }
            if (inst->operand_count == 1 && inst->operands[0].type == OPERAND_REG) {
                return 3; /* FF /2 with REX */
            }
            return 9; /* Memory indirect */

        /* Conditional jumps */
        case INST_JA: case INST_JAE: case INST_JB: case INST_JBE:
        case INST_JE: case INST_JG: case INST_JGE: case INST_JL:
        case INST_JLE: case INST_JNE: case INST_JNZ: case INST_JNO:
        case INST_JNP: case INST_JNS: case INST_JO: case INST_JP:
        case INST_JS:
            /* 0F 80+cc cd - 6 bytes for near jump */
            return 6;

        /* LEA */
        case INST_LEA:
            if (inst->operand_count == 2 && inst->operands[1].type == OPERAND_MEM) {
                /* Check if RIP-relative addressing is used */
                if (inst->operands[1].mem.is_rip_relative) {
                    return 7; /* REX + opcode + modrm + disp32 (no SIB for RIP-relative) */
                }
            }
            return 9; /* REX + opcode + modrm + SIB + disp32 */

        /* XCHG */
        case INST_XCHG:
            if (inst->operand_count == 2 &&
                inst->operands[0].type == OPERAND_REG &&
                inst->operands[1].type == OPERAND_REG) {
                return 3;
            }
            return 9;

        /* IMUL */
        case INST_IMUL:
            return 9;

        /* MUL, DIV, IDIV */
        case INST_MUL: case INST_DIV: case INST_IDIV:
            if (inst->operand_count == 1) {
                if (inst->operands[0].type == OPERAND_REG) {
                    return 3;
                }
                if (inst->operands[0].type == OPERAND_MEM) {
                    return 9;
                }
            }
            return 9;

        /* TEST */
        case INST_TEST:
            return 9;

        /* Shifts */
        case INST_SHL: case INST_SHR: case INST_SAL: case INST_SAR:
        case INST_ROL: case INST_ROR: case INST_RCL: case INST_RCR:
            if (inst->operand_count == 2) {
                if (inst->operands[1].type == OPERAND_IMM) {
                    return 4; /* REX + opcode + modrm + imm8 */
                }
                if (inst->operands[1].type == OPERAND_REG &&
                    inst->operands[1].reg == 1) { /* CL register (lower 8-bit of RCX) */
                    return 3; /* REX + opcode + modrm */
                }
            }
            return 9;

        /* SETcc */
        case INST_SETA: case INST_SETAE: case INST_SETB: case INST_SETBE:
        case INST_SETE: case INST_SETG: case INST_SETGE: case INST_SETL:
        case INST_SETLE: case INST_SETNE: case INST_SETNO: case INST_SETNP:
        case INST_SETNS: case INST_SETO: case INST_SETP: case INST_SETS:
            return 4; /* REX + 0F 90+cc /r */

        /* CMOVcc */
        case INST_CMOVA: case INST_CMOVAE: case INST_CMOVB: case INST_CMOVBE:
        case INST_CMOVE: case INST_CMOVG: case INST_CMOVGE: case INST_CMOVL:
        case INST_CMOVLE: case INST_CMOVNE: case INST_CMOVNO: case INST_CMOVNP:
        case INST_CMOVNS: case INST_CMOVO: case INST_CMOVP: case INST_CMOVS:
            return 9; /* REX + 0F 40+cc /r with possible SIB */

        /* MOVSX/MOVZX */
        case INST_MOVSX: case INST_MOVZX:
            return 9;

        default:
            return 15; /* Conservative default */
    }
}

/* ============================================================================
 * MAIN ASSEMBLY
 * ============================================================================ */

int asm_assemble(assembler_context_t *ctx, const char *source) {
    int count = 0;
    /* Use context-aware parsing to enable macro support */
    parsed_instruction_t *insts = parse_source_with_context(ctx, source, &count);
    if (!insts) {
        fprintf(stderr, "Error: Parsing failed\n");
        return -1;
    }

    ctx->current_address = ctx->base_address;
    ctx->line_map_count = 0;

    /* First pass: collect labels and calculate addresses */
    /* We do two passes through the instructions - first for text, then for data */
    
    /* Pass 1a: Calculate text section size */
    uint64_t text_size = 0;
    ctx->current_section = 0;
    for (int i = 0; i < count; i++) {
        parsed_instruction_t *inst = &insts[i];

        /* Handle section directives */
        if (inst->type == INST_SECTION) {
            if (inst->operand_count > 0 && inst->operands[0].type == OPERAND_LABEL) {
                const char *section_name = inst->operands[0].label;
                if (strcmp(section_name, ".data") == 0 ||
                    strcmp(section_name, "data") == 0 ||
                    strcmp(section_name, ".bss") == 0 ||
                    strcmp(section_name, "bss") == 0 ||
                    strcmp(section_name, ".rodata") == 0 ||
                    strcmp(section_name, "rodata") == 0) {
                    ctx->current_section = 1;
                } else {
                    ctx->current_section = 0;
                }
            }
            continue;
        }

        if (ctx->current_section == 0) {
            /* Text section */
            if (inst->type == INST_DB || inst->type == INST_DW ||
                inst->type == INST_DD || inst->type == INST_DQ) {
                int size = (inst->type == INST_DB) ? 1 :
                          (inst->type == INST_DW) ? 2 :
                          (inst->type == INST_DD) ? 4 : 8;
                text_size += size * inst->operand_count;
            } else if (inst->type != INST_GLOBAL && inst->type != INST_EXTERN &&
                       inst->type != INST_NOP) {
                text_size += estimate_instruction_size(inst);
            }
        }
    }

    /* Data section starts right after text section (contiguous) */
    uint64_t data_address = ctx->base_address + text_size;
    
    /* Pass 1b: Record all label addresses */
    ctx->current_section = 0;
    ctx->current_address = ctx->base_address;
    uint64_t current_data_addr = data_address;
    
    for (int i = 0; i < count; i++) {
        parsed_instruction_t *inst = &insts[i];

        /* Handle section directives */
        if (inst->type == INST_SECTION) {
            if (inst->operand_count > 0 && inst->operands[0].type == OPERAND_LABEL) {
                const char *section_name = inst->operands[0].label;
                if (strcmp(section_name, ".data") == 0 ||
                    strcmp(section_name, "data") == 0 ||
                    strcmp(section_name, ".bss") == 0 ||
                    strcmp(section_name, "bss") == 0 ||
                    strcmp(section_name, ".rodata") == 0 ||
                    strcmp(section_name, "rodata") == 0) {
                    ctx->current_section = 1;
                } else {
                    ctx->current_section = 0;
                }
            }
            continue;
        }

        /* Record label with appropriate address */
        if (inst->has_label) {
            uint64_t addr = (ctx->current_section == 0) ? ctx->current_address : current_data_addr;
            add_symbol(ctx, inst->label, addr, false, false, ctx->current_section);
            for (int j = 0; j < ctx->symbol_count; j++) {
                if (strcmp(ctx->symbols[j].name, inst->label) == 0) {
                    if (ctx->symbols[j].decl_line == 0) {
                        ctx->symbols[j].decl_line = inst->line_number > 0 ? inst->line_number : 1;
                        ctx->symbols[j].decl_column = inst->label_column > 0 ? inst->label_column : 1;
                        if (ctx->current_section == 0 && strcmp(inst->label, "_start") == 0) {
                            ctx->symbols[j].is_function = true;
                        }
                    }
                    break;
                }
            }
        }

        /* Calculate size based on instruction type and section */
        if (inst->type == INST_DB || inst->type == INST_DW ||
            inst->type == INST_DD || inst->type == INST_DQ) {
            int size = (inst->type == INST_DB) ? 1 :
                      (inst->type == INST_DW) ? 2 :
                      (inst->type == INST_DD) ? 4 : 8;
            /* Calculate total bytes - handle string literals for db */
            int total_bytes = 0;
            for (int j = 0; j < inst->operand_count; j++) {
                if (inst->operands[j].type == OPERAND_STRING && inst->type == INST_DB) {
                    total_bytes += (int)strlen(inst->operands[j].string);
                } else {
                    total_bytes += size;
                }
            }
            if (ctx->current_section == 0) {
                ctx->current_address += total_bytes;
            } else {
                current_data_addr += total_bytes;
            }
        } else if (inst->type == INST_RESB || inst->type == INST_RESW ||
                   inst->type == INST_RESD || inst->type == INST_RESQ) {
            /* Reserve uninitialized space */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_IMM) {
                int size = (inst->type == INST_RESB) ? 1 :
                          (inst->type == INST_RESW) ? 2 :
                          (inst->type == INST_RESD) ? 4 : 8;
                int64_t reserved = inst->operands[0].immediate * size;
                if (ctx->current_section == 0) {
                    ctx->current_address += reserved;
                } else {
                    current_data_addr += reserved;
                }
            }
        } else if (inst->type == INST_EQU) {
            /* equ doesn't take up space - define symbol if not already done */
            if (inst->has_label && inst->operand_count >= 1 &&
                inst->operands[0].type == OPERAND_IMM) {
                /* Check if symbol already exists */
                bool exists = false;
                for (int j = 0; j < ctx->symbol_count; j++) {
                    if (strcmp(ctx->symbols[j].name, inst->label) == 0) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    /* equ defines an absolute constant, not an address */
                    add_symbol(ctx, inst->label, (uint64_t)inst->operands[0].immediate,
                              false, false, -1);  /* section -1 indicates constant */
                }
            }
        } else if (inst->type == INST_ALIGN) {
            /* Align directive - calculate padding bytes */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_IMM) {
                int alignment = (int)inst->operands[0].immediate;
                if (alignment > 0 && (alignment & (alignment - 1)) == 0) {  /* Power of 2 */
                    uint64_t current_addr = (ctx->current_section == 0) ?
                                           ctx->current_address : current_data_addr;
                    uint64_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
                    int padding = (int)(aligned_addr - current_addr);
                    if (ctx->current_section == 0) {
                        ctx->current_address += padding;
                    } else {
                        current_data_addr += padding;
                    }
                }
            }
        } else if (inst->type == INST_INCBIN) {
            /* Include binary file - add file size to address */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_STRING) {
                const char *filename = inst->operands[0].string;
                FILE *fp = fopen(filename, "rb");
                if (fp) {
                    fseek(fp, 0, SEEK_END);
                    long file_size = ftell(fp);
                    fclose(fp);
                    if (file_size > 0) {
                        if (ctx->current_section == 0) {
                            ctx->current_address += file_size;
                        } else {
                            current_data_addr += file_size;
                        }
                    }
                }
            }
        } else if (inst->type == INST_COMM || inst->type == INST_LCOMM) {
            /* Common symbol - add size to address */
            if (inst->operand_count >= 2 && inst->operands[1].type == OPERAND_IMM) {
                int64_t size = inst->operands[1].immediate;
                int64_t alignment = (inst->operand_count >= 3) ?
                                   inst->operands[2].immediate : 1;
                
                if (size > 0) {
                    uint64_t *addr = (ctx->current_section == 0) ?
                                    &ctx->current_address : &current_data_addr;
                    /* Apply alignment */
                    if (alignment > 1) {
                        *addr = (*addr + alignment - 1) & ~(alignment - 1);
                    }
                    *addr += size;
                }
            }
        } else if (inst->type != INST_GLOBAL && inst->type != INST_EXTERN &&
                   inst->type != INST_NOP && inst->type != INST_TIMES) {
            if (ctx->current_section == 0) {
                ctx->current_address += estimate_instruction_size(inst);
            }
        }
    }

    /* Second pass: generate text section code */
    ctx->current_address = ctx->base_address;
    ctx->current_section = 0;
    ctx->text_size = 0;
    ctx->fixup_count = 0;

    for (int i = 0; i < count; i++) {
        parsed_instruction_t *inst = &insts[i];

        /* Handle section directives */
        if (inst->type == INST_SECTION) {
            if (inst->operand_count > 0 && inst->operands[0].type == OPERAND_LABEL) {
                const char *section_name = inst->operands[0].label;
                if (strcmp(section_name, ".data") == 0 ||
                    strcmp(section_name, "data") == 0 ||
                    strcmp(section_name, ".bss") == 0 ||
                    strcmp(section_name, "bss") == 0 ||
                    strcmp(section_name, ".rodata") == 0 ||
                    strcmp(section_name, "rodata") == 0) {
                    ctx->current_section = 1;
                } else {
                    ctx->current_section = 0;
                }
            }
            continue;
        }

        /* Skip data section in text pass */
        if (ctx->current_section == 1) {
            continue;
        }

        /* Update label addresses */
        if (inst->has_label) {
            for (int j = 0; j < ctx->symbol_count; j++) {
                if (strcmp(ctx->symbols[j].name, inst->label) == 0) {
                    ctx->symbols[j].address = ctx->current_address;
                    /* Keep hash table in sync for fixup resolution lookups. */
                    symbol_hash_update(ctx, inst->label, ctx->current_address,
                                       ctx->symbols[j].is_global,
                                       ctx->symbols[j].is_external,
                                       ctx->symbols[j].section);
                }
            }
        }

        /* Record address for this instruction */
        inst->address = ctx->current_address;

        /* Handle pseudo-instructions */
        if (inst->type == INST_GLOBAL) {
            for (int j = 0; j < inst->operand_count; j++) {
                if (inst->operands[j].type == OPERAND_LABEL) {
                    for (int k = 0; k < ctx->symbol_count; k++) {
                        if (strcmp(ctx->symbols[k].name, inst->operands[j].label) == 0) {
                            ctx->symbols[k].is_global = true;
                            if (ctx->symbols[k].section == 0) {
                                ctx->symbols[k].is_function = true;
                            }
                        }
                    }
                }
            }
            continue;
        }

        if (inst->type == INST_CALL && inst->operand_count > 0 &&
            inst->operands[0].type == OPERAND_LABEL) {
            for (int k = 0; k < ctx->symbol_count; k++) {
                if (strcmp(ctx->symbols[k].name, inst->operands[0].label) == 0 &&
                    ctx->symbols[k].section == 0) {
                    ctx->symbols[k].is_function = true;
                    break;
                }
            }
        }

        if (inst->type == INST_EXTERN) {
            for (int j = 0; j < inst->operand_count; j++) {
                if (inst->operands[j].type == OPERAND_LABEL) {
                    add_symbol(ctx, inst->operands[j].label, 0, false, true, 0);
                }
            }
            continue;
        }

        if (inst->type == INST_NOP && inst->operand_count == 0 && inst->has_label) {
            /* Label-only line - skip encoding but don't advance address */
            continue;
        }

        if (inst->type == INST_ALIGN) {
            /* Align directive handled by encoder */
        }

        if (ctx->line_map_count < MAX_LINE_MAP) {
            ctx->line_map[ctx->line_map_count].address = inst->address;
            ctx->line_map[ctx->line_map_count].line = inst->line_number > 0 ? inst->line_number : 1;
            ctx->line_map_count++;
        }

        /* Encode instruction */
        if (encode_instruction(ctx, inst) < 0) {
            fprintf(stderr, "Error: Failed to encode instruction at line %d\n", inst->line_number);
            free_instructions(insts);
            return -1;
        }
    }

    /* Third pass: generate data section */
    ctx->current_section = 0;
    ctx->data_size = 0;
    /* Data section starts right after text section */
    ctx->current_address = ctx->base_address + ctx->text_size;
    
    for (int i = 0; i < count; i++) {
        parsed_instruction_t *inst = &insts[i];

        /* Handle section directives */
        if (inst->type == INST_SECTION) {
            if (inst->operand_count > 0 && inst->operands[0].type == OPERAND_LABEL) {
                const char *section_name = inst->operands[0].label;
                if (strcmp(section_name, ".data") == 0 ||
                    strcmp(section_name, "data") == 0 ||
                    strcmp(section_name, ".bss") == 0 ||
                    strcmp(section_name, "bss") == 0 ||
                    strcmp(section_name, ".rodata") == 0 ||
                    strcmp(section_name, "rodata") == 0) {
                    ctx->current_section = 1;
                } else {
                    ctx->current_section = 0;
                }
            }
            continue;
        }

        /* Only process data section */
        if (ctx->current_section == 0) {
            continue;
        }

        /* Update label addresses for data labels only. */
        if (inst->has_label) {
            for (int j = 0; j < ctx->symbol_count; j++) {
                if (strcmp(ctx->symbols[j].name, inst->label) == 0) {
                    ctx->symbols[j].address = ctx->current_address;
                    symbol_hash_update(ctx, inst->label, ctx->current_address,
                                       ctx->symbols[j].is_global,
                                       ctx->symbols[j].is_external,
                                       ctx->symbols[j].section);
                }
            }
        }

        /* Encode data directive */
        if (inst->type == INST_DB || inst->type == INST_DW ||
            inst->type == INST_DD || inst->type == INST_DQ) {
            for (int j = 0; j < inst->operand_count; j++) {
                if (inst->operands[j].type == OPERAND_IMM) {
                    if (inst->type == INST_DB) {
                        emit_byte(ctx, (uint8_t)inst->operands[j].immediate);
                    } else if (inst->type == INST_DW) {
                        uint16_t v = (uint16_t)inst->operands[j].immediate;
                        emit_byte(ctx, v & 0xFF);
                        emit_byte(ctx, (v >> 8) & 0xFF);
                    } else if (inst->type == INST_DD) {
                        uint32_t v = (uint32_t)inst->operands[j].immediate;
                        emit_byte(ctx, v & 0xFF);
                        emit_byte(ctx, (v >> 8) & 0xFF);
                        emit_byte(ctx, (v >> 16) & 0xFF);
                        emit_byte(ctx, (v >> 24) & 0xFF);
                    } else {
                        uint64_t v = (uint64_t)inst->operands[j].immediate;
                        for (int k = 0; k < 8; k++) {
                            emit_byte(ctx, (v >> (k * 8)) & 0xFF);
                        }
                    }
                } else if (inst->operands[j].type == OPERAND_STRING && inst->type == INST_DB) {
                    /* String literal for db directive */
                    const char *str = inst->operands[j].string;
                    for (size_t k = 0; k < strlen(str); k++) {
                        emit_byte(ctx, (uint8_t)str[k]);
                    }
                }
            }
        } else if (inst->type == INST_RESB || inst->type == INST_RESW ||
                   inst->type == INST_RESD || inst->type == INST_RESQ) {
            /* Reserve uninitialized space - emit zeros */
            if (inst->operand_count >= 1 && inst->operands[0].type == OPERAND_IMM) {
                int64_t count = inst->operands[0].immediate;
                int size = (inst->type == INST_RESB) ? 1 :
                          (inst->type == INST_RESW) ? 2 :
                          (inst->type == INST_RESD) ? 4 : 8;
                for (int64_t k = 0; k < count * size; k++) {
                    emit_byte(ctx, 0);
                }
            }
        } else if (inst->type == INST_ALIGN) {
            /* Align directive - handled by encoder */
            if (encode_instruction(ctx, inst) < 0) {
                fprintf(stderr, "Error: Failed to encode align at line %d\n", inst->line_number);
                free_instructions(insts);
                return -1;
            }
        } else if (inst->type == INST_INCBIN) {
            /* Include binary file - handled by encoder */
            if (encode_instruction(ctx, inst) < 0) {
                fprintf(stderr, "Error: Failed to encode incbin at line %d\n", inst->line_number);
                free_instructions(insts);
                return -1;
            }
        } else if (inst->type == INST_COMM || inst->type == INST_LCOMM) {
            /* Common symbol - handled by encoder */
            if (encode_instruction(ctx, inst) < 0) {
                fprintf(stderr, "Error: Failed to encode %s at line %d\n",
                        (inst->type == INST_COMM) ? ".comm" : ".lcomm",
                        inst->line_number);
                free_instructions(insts);
                return -1;
            }
        }
    }

    /* Fourth pass: resolve fixups */
    if (resolve_fixups(ctx) < 0) {
        free_instructions(insts);
        return -1;
    }

    free_instructions(insts);
    return 0;
}

int asm_assemble_file(assembler_context_t *ctx, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = malloc(size + 1);
    if (!source) {
        fclose(f);
        return -1;
    }

    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    strncpy(ctx->current_filename, filename, MAX_FILEPATH_LENGTH - 1);
    ctx->current_filename[MAX_FILEPATH_LENGTH - 1] = '\0';

    int result = asm_assemble(ctx, source);
    free(source);
    return result;
}

/* ============================================================================
 * OUTPUT FORMATS
 * ============================================================================ */

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
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

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
} Elf64_Shdr;

#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_STRTAB      3

#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4

static size_t align_up(size_t value, size_t align) {
    if (align == 0) return value;
    return (value + align - 1) & ~(align - 1);
}

static bool append_u8_checked(uint8_t *buf, size_t *len, size_t capacity, uint8_t value) {
    if (*len + 1 > capacity) return false;
    buf[(*len)++] = value;
    return true;
}

static bool append_le16_buf_checked(uint8_t *buf, size_t *len, size_t capacity, uint16_t value) {
    if (*len + 2 > capacity) return false;
    buf[(*len)++] = (uint8_t)(value & 0xFF);
    buf[(*len)++] = (uint8_t)((value >> 8) & 0xFF);
    return true;
}

static bool append_le32_buf_checked(uint8_t *buf, size_t *len, size_t capacity, uint32_t value) {
    if (*len + 4 > capacity) return false;
    buf[(*len)++] = (uint8_t)(value & 0xFF);
    buf[(*len)++] = (uint8_t)((value >> 8) & 0xFF);
    buf[(*len)++] = (uint8_t)((value >> 16) & 0xFF);
    buf[(*len)++] = (uint8_t)((value >> 24) & 0xFF);
    return true;
}

static bool append_le64_buf_checked(uint8_t *buf, size_t *len, size_t capacity, uint64_t value) {
    if (*len + 8 > capacity) return false;
    for (int i = 0; i < 8; i++) {
        buf[(*len)++] = (uint8_t)((value >> (8 * i)) & 0xFF);
    }
    return true;
}

static bool write_le32_at_checked(uint8_t *buf, size_t offset, size_t capacity, uint32_t value) {
    if (offset + 4 > capacity) return false;
    buf[offset + 0] = (uint8_t)(value & 0xFF);
    buf[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    buf[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
    buf[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
    return true;
}

static bool append_uleb128_checked(uint8_t *buf, size_t *len, size_t capacity, uint32_t value) {
    do {
        uint8_t byte = (uint8_t)(value & 0x7F);
        value >>= 7;
        if (value != 0) byte |= 0x80;
        if (!append_u8_checked(buf, len, capacity, byte)) return false;
    } while (value != 0);
    return true;
}

static bool append_sleb128_checked(uint8_t *buf, size_t *len, size_t capacity, int32_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = (uint8_t)(value & 0x7F);
        bool sign_bit = (byte & 0x40) != 0;
        value >>= 7;
        if ((value == 0 && !sign_bit) || (value == -1 && sign_bit)) {
            more = false;
        } else {
            byte |= 0x80;
        }
        if (!append_u8_checked(buf, len, capacity, byte)) return false;
    }
    return true;
}

static bool append_bytes_checked(uint8_t *buf, size_t *len, size_t capacity,
                                 const uint8_t *src, size_t src_len) {
    if (*len + src_len > capacity) return false;
    memcpy(buf + *len, src, src_len);
    *len += src_len;
    return true;
}

static const char *path_basename(const char *path) {
    const char *slash;
    if (!path || !path[0]) return "<memory>";
    slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static void path_dirname(const char *path, char *out, size_t out_size) {
    const char *slash;
    if (!out || out_size == 0) return;
    if (!path || !path[0]) {
        snprintf(out, out_size, ".");
        return;
    }
    slash = strrchr(path, '/');
    if (!slash) {
        snprintf(out, out_size, ".");
        return;
    }
    if (slash == path) {
        snprintf(out, out_size, "/");
        return;
    }
    size_t len = (size_t)(slash - path);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

/* Write ELF64 executable with proper layout:
 * - Headers at start of file
 * - Code at page-aligned offset (0x1000)
 * - Entry point = base + code_offset + symbol_offset
 */
int asm_write_elf64(assembler_context_t *ctx, const char *filename) {
    const bool emit_dwarf = ctx->emit_debug_map;
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return -1;
    }

    size_t pagesize = 4096;
    
    /* Place code at page-aligned offset in file */
    size_t code_offset = pagesize;  /* 0x1000 */
    
    /* Build optional DWARF sections when -g is enabled. */
    uint8_t debug_info[16384] = {0};
    uint8_t debug_abbrev[256] = {0};
    uint8_t debug_line[65536] = {0};
    uint8_t debug_str[2048] = {0};
    size_t debug_info_size = 0;
    size_t debug_abbrev_size = 0;
    size_t debug_line_size = 0;
    size_t debug_str_size = 0;
    uint32_t off_producer = 0;
    uint32_t off_name = 0;
    uint32_t off_comp_dir = 0;
    uint32_t off_type_u64 = 0;
    uint32_t off_type_u64_ptr = 0;
    uint32_t symbol_str_off[MAX_SYMBOLS] = {0};
    bool symbol_str_has_off[MAX_SYMBOLS] = {0};
    int function_symbol_indices[MAX_SYMBOLS] = {0};
    int function_symbol_count = 0;
    int label_symbol_indices[MAX_SYMBOLS] = {0};
    int label_symbol_count = 0;

#define DW_APPEND_U8(buf, len_ptr, value) \
    do { if (!append_u8_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_LE16(buf, len_ptr, value) \
    do { if (!append_le16_buf_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_LE32(buf, len_ptr, value) \
    do { if (!append_le32_buf_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_LE64(buf, len_ptr, value) \
    do { if (!append_le64_buf_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_ULEB(buf, len_ptr, value) \
    do { if (!append_uleb128_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_SLEB(buf, len_ptr, value) \
    do { if (!append_sleb128_checked((buf), (len_ptr), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_WRITE_LE32_AT(buf, offset, value) \
    do { if (!write_le32_at_checked((buf), (offset), sizeof(buf), (value))) goto dwarf_overflow; } while (0)
#define DW_APPEND_BYTES(buf, len_ptr, src, src_len) \
    do { if (!append_bytes_checked((buf), (len_ptr), sizeof(buf), (const uint8_t *)(src), (src_len))) goto dwarf_overflow; } while (0)

    if (emit_dwarf) {
        const char *producer = "x86_64-asm";
        const char *name = ctx->current_filename[0] ? ctx->current_filename : "<memory>";
        const char *base_name = path_basename(name);
        char source_dir[MAX_FILEPATH_LENGTH];
        path_dirname(name, source_dir, sizeof(source_dir));

        off_producer = (uint32_t)debug_str_size;
        DW_APPEND_BYTES(debug_str, &debug_str_size, producer, strlen(producer) + 1);

        off_name = (uint32_t)debug_str_size;
        DW_APPEND_BYTES(debug_str, &debug_str_size, name, strlen(name) + 1);

        off_comp_dir = (uint32_t)debug_str_size;
        DW_APPEND_BYTES(debug_str, &debug_str_size, source_dir, strlen(source_dir) + 1);

        off_type_u64 = (uint32_t)debug_str_size;
        DW_APPEND_BYTES(debug_str, &debug_str_size, "u64", 4);

        off_type_u64_ptr = (uint32_t)debug_str_size;
        DW_APPEND_BYTES(debug_str, &debug_str_size, "u64*", 5);

        /* Build symbol indexes and .debug_str offsets for function/label DIEs. */
        for (int i = 0; i < ctx->symbol_count; i++) {
            if (!ctx->symbols[i].is_resolved || ctx->symbols[i].is_external || ctx->symbols[i].section != 0) {
                continue;
            }
            if (ctx->symbols[i].address < ctx->base_address ||
                ctx->symbols[i].address > ctx->base_address + ctx->text_size) {
                continue;
            }
            if (!symbol_str_has_off[i]) {
                symbol_str_off[i] = (uint32_t)debug_str_size;
                symbol_str_has_off[i] = true;
                size_t name_len = strlen(ctx->symbols[i].name);
                DW_APPEND_BYTES(debug_str, &debug_str_size, ctx->symbols[i].name, name_len + 1);
            }

            if (ctx->symbols[i].is_function) {
                function_symbol_indices[function_symbol_count++] = i;
            } else {
                label_symbol_indices[label_symbol_count++] = i;
            }
        }

        /* Ensure we always emit at least one function DIE for _start if present. */
        if (function_symbol_count == 0) {
            for (int i = 0; i < ctx->symbol_count; i++) {
                if (ctx->symbols[i].section == 0 && strcmp(ctx->symbols[i].name, "_start") == 0) {
                    function_symbol_indices[function_symbol_count++] = i;
                    if (!symbol_str_has_off[i]) {
                        symbol_str_off[i] = (uint32_t)debug_str_size;
                        symbol_str_has_off[i] = true;
                        size_t name_len = strlen(ctx->symbols[i].name);
                        DW_APPEND_BYTES(debug_str, &debug_str_size, ctx->symbols[i].name, name_len + 1);
                    }
                    break;
                }
            }
        }

        /* Sort symbols by address for deterministic high_pc and DIE ordering. */
        for (int i = 0; i < function_symbol_count; i++) {
            for (int j = i + 1; j < function_symbol_count; j++) {
                int si = function_symbol_indices[i];
                int sj = function_symbol_indices[j];
                if (ctx->symbols[sj].address < ctx->symbols[si].address) {
                    int tmp = function_symbol_indices[i];
                    function_symbol_indices[i] = function_symbol_indices[j];
                    function_symbol_indices[j] = tmp;
                }
            }
        }
        for (int i = 0; i < label_symbol_count; i++) {
            for (int j = i + 1; j < label_symbol_count; j++) {
                int si = label_symbol_indices[i];
                int sj = label_symbol_indices[j];
                if (ctx->symbols[sj].address < ctx->symbols[si].address) {
                    int tmp = label_symbol_indices[i];
                    label_symbol_indices[i] = label_symbol_indices[j];
                    label_symbol_indices[j] = tmp;
                }
            }
        }

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 1);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x11);   /* DW_TAG_compile_unit */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 1);        /* has children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x25);   /* DW_AT_producer */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x13);   /* DW_AT_language */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x05);   /* DW_FORM_data2 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x03);   /* DW_AT_name */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x1b);   /* DW_AT_comp_dir */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x10);   /* DW_AT_stmt_list */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x17);   /* DW_FORM_sec_offset */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x11);   /* DW_AT_low_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x01);   /* DW_FORM_addr */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x12);   /* DW_AT_high_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x07);   /* DW_FORM_data8 */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* attr terminator */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 2);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x24);   /* DW_TAG_base_type */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* no children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x03);   /* DW_AT_name */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3E);   /* DW_AT_encoding */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_FORM_data1 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_AT_byte_size */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_FORM_data1 */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 3);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0F);   /* DW_TAG_pointer_type */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* no children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x03);   /* DW_AT_name */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_AT_byte_size */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_FORM_data1 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x49);   /* DW_AT_type */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x13);   /* DW_FORM_ref4 */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 4);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x2E);   /* DW_TAG_subprogram */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 1);        /* has children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x03);   /* DW_AT_name */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3A);   /* DW_AT_decl_file */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_FORM_data1 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3B);   /* DW_AT_decl_line */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0F);   /* DW_FORM_udata */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x39);   /* DW_AT_decl_column */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0F);   /* DW_FORM_udata */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3f);   /* DW_AT_external */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0c);   /* DW_FORM_flag */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x11);   /* DW_AT_low_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x01);   /* DW_FORM_addr */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x12);   /* DW_AT_high_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x07);   /* DW_FORM_data8 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x49);   /* DW_AT_type */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x13);   /* DW_FORM_ref4 */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 5);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_TAG_lexical_block */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* no children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x11);   /* DW_AT_low_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x01);   /* DW_FORM_addr */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x12);   /* DW_AT_high_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x07);   /* DW_FORM_data8 */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 6);      /* abbrev code */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0A);   /* DW_TAG_label */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* no children */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x03);   /* DW_AT_name */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0E);   /* DW_FORM_strp */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3A);   /* DW_AT_decl_file */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0B);   /* DW_FORM_data1 */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x3B);   /* DW_AT_decl_line */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0F);   /* DW_FORM_udata */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x39);   /* DW_AT_decl_column */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x0F);   /* DW_FORM_udata */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x11);   /* DW_AT_low_pc */
        DW_APPEND_ULEB(debug_abbrev, &debug_abbrev_size, 0x01);   /* DW_FORM_addr */
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);
        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);

        DW_APPEND_U8(debug_abbrev, &debug_abbrev_size, 0);        /* abbrev table terminator */

        size_t info_len_off = debug_info_size;
        DW_APPEND_LE32(debug_info, &debug_info_size, 0);
        DW_APPEND_LE16(debug_info, &debug_info_size, 4);          /* DWARF version */
        DW_APPEND_LE32(debug_info, &debug_info_size, 0);          /* .debug_abbrev offset */
        DW_APPEND_U8(debug_info, &debug_info_size, 8);            /* address size */
        DW_APPEND_ULEB(debug_info, &debug_info_size, 1);          /* abbrev code */
        DW_APPEND_LE32(debug_info, &debug_info_size, off_producer);
        DW_APPEND_LE16(debug_info, &debug_info_size, 0x0001);     /* DW_LANG_Asm (roadmap target) */
        DW_APPEND_LE32(debug_info, &debug_info_size, off_name);
        DW_APPEND_LE32(debug_info, &debug_info_size, off_comp_dir);
        DW_APPEND_LE32(debug_info, &debug_info_size, 0);          /* DW_AT_stmt_list */
        DW_APPEND_LE64(debug_info, &debug_info_size, ctx->base_address);
        DW_APPEND_LE64(debug_info, &debug_info_size, (uint64_t)ctx->text_size);

        /* Emit shared base/pointer type DIEs so subprograms can carry DW_AT_type refs. */
        uint32_t base_type_ref_off = (uint32_t)debug_info_size;
        DW_APPEND_ULEB(debug_info, &debug_info_size, 2);          /* base type DIE */
        DW_APPEND_LE32(debug_info, &debug_info_size, off_type_u64);
        DW_APPEND_U8(debug_info, &debug_info_size, 0x07);         /* DW_ATE_unsigned */
        DW_APPEND_U8(debug_info, &debug_info_size, 8);

        uint32_t pointer_type_ref_off = (uint32_t)debug_info_size;
        DW_APPEND_ULEB(debug_info, &debug_info_size, 3);          /* pointer type DIE */
        DW_APPEND_LE32(debug_info, &debug_info_size, off_type_u64_ptr);
        DW_APPEND_U8(debug_info, &debug_info_size, 8);
        DW_APPEND_LE32(debug_info, &debug_info_size, base_type_ref_off);

        /* Emit one subprogram DIE per text symbol. */
        for (int idx = 0; idx < function_symbol_count; idx++) {
            int sym_idx = function_symbol_indices[idx];
            uint64_t low_pc = ctx->symbols[sym_idx].address;
            uint64_t next_addr = (idx + 1 < function_symbol_count)
                                 ? ctx->symbols[function_symbol_indices[idx + 1]].address
                                 : (ctx->base_address + ctx->text_size);
            uint64_t span = (next_addr > low_pc) ? (next_addr - low_pc) : 0;

            DW_APPEND_ULEB(debug_info, &debug_info_size, 4);      /* subprogram DIE */
            DW_APPEND_LE32(debug_info, &debug_info_size, symbol_str_off[sym_idx]);
            DW_APPEND_U8(debug_info, &debug_info_size, 1);        /* file index from line table */
            DW_APPEND_ULEB(debug_info, &debug_info_size,
                           (uint32_t)(ctx->symbols[sym_idx].decl_line > 0 ? ctx->symbols[sym_idx].decl_line : 1));
            DW_APPEND_ULEB(debug_info, &debug_info_size,
                           (uint32_t)(ctx->symbols[sym_idx].decl_column > 0 ? ctx->symbols[sym_idx].decl_column : 1));
            DW_APPEND_U8(debug_info, &debug_info_size, ctx->symbols[sym_idx].is_global ? 1 : 0);
            DW_APPEND_LE64(debug_info, &debug_info_size, low_pc);
            DW_APPEND_LE64(debug_info, &debug_info_size, span);
            DW_APPEND_LE32(debug_info, &debug_info_size, pointer_type_ref_off);

            /* Minimal lexical scope to represent a function body block. */
            DW_APPEND_ULEB(debug_info, &debug_info_size, 5);      /* lexical block DIE */
            DW_APPEND_LE64(debug_info, &debug_info_size, low_pc);
            DW_APPEND_LE64(debug_info, &debug_info_size, span);
            DW_APPEND_U8(debug_info, &debug_info_size, 0);        /* end of subprogram children */
        }

        /* Emit non-function text labels as DW_TAG_label DIEs. */
        for (int idx = 0; idx < label_symbol_count; idx++) {
            int sym_idx = label_symbol_indices[idx];
            DW_APPEND_ULEB(debug_info, &debug_info_size, 6);      /* label DIE */
            DW_APPEND_LE32(debug_info, &debug_info_size, symbol_str_off[sym_idx]);
            DW_APPEND_U8(debug_info, &debug_info_size, 1);        /* file index from line table */
            DW_APPEND_ULEB(debug_info, &debug_info_size,
                           (uint32_t)(ctx->symbols[sym_idx].decl_line > 0 ? ctx->symbols[sym_idx].decl_line : 1));
            DW_APPEND_ULEB(debug_info, &debug_info_size,
                           (uint32_t)(ctx->symbols[sym_idx].decl_column > 0 ? ctx->symbols[sym_idx].decl_column : 1));
            DW_APPEND_LE64(debug_info, &debug_info_size, ctx->symbols[sym_idx].address);
        }

        DW_APPEND_U8(debug_info, &debug_info_size, 0);            /* end of children */
        DW_WRITE_LE32_AT(debug_info, info_len_off, (uint32_t)(debug_info_size - 4));

        size_t line_len_off = debug_line_size;
        size_t line_hdr_len_off;
        size_t line_hdr_start;
        DW_APPEND_LE32(debug_line, &debug_line_size, 0);
        DW_APPEND_LE16(debug_line, &debug_line_size, 4);          /* DWARF version */
        line_hdr_len_off = debug_line_size;
        DW_APPEND_LE32(debug_line, &debug_line_size, 0);
        line_hdr_start = debug_line_size;
        DW_APPEND_U8(debug_line, &debug_line_size, 1);            /* min instruction length */
        DW_APPEND_U8(debug_line, &debug_line_size, 1);            /* max ops per instruction */
        DW_APPEND_U8(debug_line, &debug_line_size, 1);            /* default is_stmt */
        DW_APPEND_U8(debug_line, &debug_line_size, (uint8_t)-5);  /* line_base */
        DW_APPEND_U8(debug_line, &debug_line_size, 14);           /* line_range */
        DW_APPEND_U8(debug_line, &debug_line_size, 13);           /* opcode_base */
        for (int i = 0; i < 12; i++) DW_APPEND_U8(debug_line, &debug_line_size, 0);

        DW_APPEND_BYTES(debug_line, &debug_line_size, source_dir, strlen(source_dir) + 1);
        DW_APPEND_U8(debug_line, &debug_line_size, 0);            /* include dirs terminator */

        DW_APPEND_BYTES(debug_line, &debug_line_size, base_name, strlen(base_name) + 1);
        DW_APPEND_ULEB(debug_line, &debug_line_size, 1);          /* directory index */
        DW_APPEND_ULEB(debug_line, &debug_line_size, 0);          /* mtime */
        DW_APPEND_ULEB(debug_line, &debug_line_size, 0);          /* length */
        DW_APPEND_U8(debug_line, &debug_line_size, 0);            /* file names terminator */

        DW_WRITE_LE32_AT(debug_line, line_hdr_len_off, (uint32_t)(debug_line_size - line_hdr_start));

        DW_APPEND_U8(debug_line, &debug_line_size, 0);            /* extended opcode */
        DW_APPEND_U8(debug_line, &debug_line_size, 9);            /* payload length */
        DW_APPEND_U8(debug_line, &debug_line_size, 2);            /* DW_LNE_set_address */
        DW_APPEND_LE64(debug_line, &debug_line_size, ctx->base_address);

        uint64_t last_addr = ctx->base_address;
        int last_line = 1;
        for (int i = 0; i < ctx->line_map_count; i++) {
            uint64_t target_addr = ctx->line_map[i].address;
            int target_line = ctx->line_map[i].line > 0 ? ctx->line_map[i].line : 1;

            if (target_addr > last_addr) {
                DW_APPEND_U8(debug_line, &debug_line_size, 2);    /* DW_LNS_advance_pc */
                DW_APPEND_ULEB(debug_line, &debug_line_size, (uint32_t)(target_addr - last_addr));
                last_addr = target_addr;
            }

            if (target_line != last_line) {
                DW_APPEND_U8(debug_line, &debug_line_size, 3);    /* DW_LNS_advance_line */
                DW_APPEND_SLEB(debug_line, &debug_line_size, target_line - last_line);
                last_line = target_line;
            }

            DW_APPEND_U8(debug_line, &debug_line_size, 1);        /* DW_LNS_copy */
        }

        if (last_addr < ctx->base_address + ctx->text_size) {
            DW_APPEND_U8(debug_line, &debug_line_size, 2);        /* DW_LNS_advance_pc */
            DW_APPEND_ULEB(debug_line, &debug_line_size,
                           (uint32_t)((ctx->base_address + ctx->text_size) - last_addr));
            DW_APPEND_U8(debug_line, &debug_line_size, 1);        /* DW_LNS_copy */
        }

        DW_APPEND_U8(debug_line, &debug_line_size, 0);            /* extended opcode */
        DW_APPEND_U8(debug_line, &debug_line_size, 1);            /* payload length */
        DW_APPEND_U8(debug_line, &debug_line_size, 1);            /* DW_LNE_end_sequence */
        DW_WRITE_LE32_AT(debug_line, line_len_off, (uint32_t)(debug_line_size - 4));
    }

#undef DW_APPEND_U8
#undef DW_APPEND_LE16
#undef DW_APPEND_LE32
#undef DW_APPEND_LE64
#undef DW_APPEND_ULEB
#undef DW_APPEND_SLEB
#undef DW_WRITE_LE32_AT
#undef DW_APPEND_BYTES

    /* ELF header */
    Elf64_Ehdr ehdr = {0};
    ehdr.e_ident[0] = ELFMAG0;
    ehdr.e_ident[1] = ELFMAG1;
    ehdr.e_ident[2] = ELFMAG2;
    ehdr.e_ident[3] = ELFMAG3;
    ehdr.e_ident[4] = ELFCLASS64;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_ident[7] = 0; /* OS/ABI */
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = 1;

    /* Find entry point (_start) */
    uint64_t entry = ctx->base_address;
    for (int i = 0; i < ctx->symbol_count; i++) {
        if (strcmp(ctx->symbols[i].name, "_start") == 0) {
            entry = ctx->symbols[i].address;
            break;
        }
    }

    ehdr.e_entry = entry;
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = 1;
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;

    /* Program header - single segment for both text and data at base_address */
    size_t total_size = ctx->text_size + ctx->data_size;
    size_t total_aligned = (total_size + pagesize - 1) & ~(pagesize - 1);
    
    Elf64_Phdr phdr = {0};
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_X | PF_R | PF_W;  /* Executable, readable, writable */
    phdr.p_offset = code_offset;
    phdr.p_vaddr = ctx->base_address;
    phdr.p_paddr = ctx->base_address;
    phdr.p_filesz = total_size;
    phdr.p_memsz = total_aligned;
    phdr.p_align = pagesize;

    const char shstrtab[] = "\0.text\0.data\0.shstrtab\0.debug_info\0.debug_abbrev\0.debug_line\0.debug_str\0";
    const uint32_t sh_name_text = 1;
    const uint32_t sh_name_data = 7;
    const uint32_t sh_name_shstrtab = 13;
    const uint32_t sh_name_debug_info = 23;
    const uint32_t sh_name_debug_abbrev = 35;
    const uint32_t sh_name_debug_line = 49;
    const uint32_t sh_name_debug_str = 61;
    const size_t shstrtab_size = sizeof(shstrtab);

    size_t debug_info_offset = 0;
    size_t debug_abbrev_offset = 0;
    size_t debug_line_offset = 0;
    size_t debug_str_offset = 0;
    size_t shstrtab_offset = 0;
    size_t shoff = 0;

    if (emit_dwarf) {
        size_t cursor = code_offset + total_aligned;
        debug_info_offset = cursor;
        cursor += debug_info_size;
        debug_abbrev_offset = cursor;
        cursor += debug_abbrev_size;
        debug_line_offset = cursor;
        cursor += debug_line_size;
        debug_str_offset = cursor;
        cursor += debug_str_size;
        shstrtab_offset = cursor;
        cursor += shstrtab_size;
        shoff = align_up(cursor, 8);

        ehdr.e_shoff = shoff;
        ehdr.e_shentsize = sizeof(Elf64_Shdr);
        ehdr.e_shnum = 8;
        ehdr.e_shstrndx = 3;
    }

    fwrite(&ehdr, sizeof(ehdr), 1, f);
    fwrite(&phdr, sizeof(phdr), 1, f);

    /* Pad to code offset */
    size_t header_size = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    for (size_t i = header_size; i < code_offset; i++) {
        fputc(0, f);
    }

    /* Write text section followed immediately by data section */
    fwrite(ctx->text_section, 1, ctx->text_size, f);
    
    if (ctx->data_size > 0) {
        fwrite(ctx->data_section, 1, ctx->data_size, f);
    }

    /* Pad to page boundary */
    size_t padding = total_aligned - total_size;
    for (size_t i = 0; i < padding; i++) {
        fputc(0, f);
    }

    if (emit_dwarf) {
        fwrite(debug_info, 1, debug_info_size, f);
        fwrite(debug_abbrev, 1, debug_abbrev_size, f);
        fwrite(debug_line, 1, debug_line_size, f);
        fwrite(debug_str, 1, debug_str_size, f);
        fwrite(shstrtab, 1, shstrtab_size, f);

        size_t current = code_offset + total_aligned +
                        debug_info_size + debug_abbrev_size +
                        debug_line_size + debug_str_size + shstrtab_size;
        while (current < shoff) {
            fputc(0, f);
            current++;
        }

        Elf64_Shdr shdrs[8];
        memset(shdrs, 0, sizeof(shdrs));

        shdrs[1].sh_name = sh_name_text;
        shdrs[1].sh_type = SHT_PROGBITS;
        shdrs[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        shdrs[1].sh_addr = ctx->base_address;
        shdrs[1].sh_offset = code_offset;
        shdrs[1].sh_size = ctx->text_size;
        shdrs[1].sh_addralign = 16;

        shdrs[2].sh_name = sh_name_data;
        shdrs[2].sh_type = SHT_PROGBITS;
        shdrs[2].sh_flags = SHF_ALLOC | SHF_WRITE;
        shdrs[2].sh_addr = ctx->base_address + ctx->text_size;
        shdrs[2].sh_offset = code_offset + ctx->text_size;
        shdrs[2].sh_size = ctx->data_size;
        shdrs[2].sh_addralign = 8;

        shdrs[3].sh_name = sh_name_shstrtab;
        shdrs[3].sh_type = SHT_STRTAB;
        shdrs[3].sh_offset = shstrtab_offset;
        shdrs[3].sh_size = shstrtab_size;
        shdrs[3].sh_addralign = 1;

        shdrs[4].sh_name = sh_name_debug_info;
        shdrs[4].sh_type = SHT_PROGBITS;
        shdrs[4].sh_offset = debug_info_offset;
        shdrs[4].sh_size = debug_info_size;
        shdrs[4].sh_link = 7; /* .debug_str */
        shdrs[4].sh_info = 0;
        shdrs[4].sh_addralign = 1;

        shdrs[5].sh_name = sh_name_debug_abbrev;
        shdrs[5].sh_type = SHT_PROGBITS;
        shdrs[5].sh_offset = debug_abbrev_offset;
        shdrs[5].sh_size = debug_abbrev_size;
        shdrs[5].sh_addralign = 1;

        shdrs[6].sh_name = sh_name_debug_line;
        shdrs[6].sh_type = SHT_PROGBITS;
        shdrs[6].sh_offset = debug_line_offset;
        shdrs[6].sh_size = debug_line_size;
        shdrs[6].sh_addralign = 1;

        shdrs[7].sh_name = sh_name_debug_str;
        shdrs[7].sh_type = SHT_STRTAB;
        shdrs[7].sh_offset = debug_str_offset;
        shdrs[7].sh_size = debug_str_size;
        shdrs[7].sh_addralign = 1;

        fwrite(shdrs, sizeof(shdrs), 1, f);
    }

    fclose(f);
    return 0;

dwarf_overflow:
    fprintf(stderr, "Error: DWARF section data exceeded internal buffer capacity\n");
    fclose(f);
    return -1;
}

int asm_write_binary(assembler_context_t *ctx, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return -1;
    }

    fwrite(ctx->text_section, 1, ctx->text_size, f);
    fclose(f);
    return 0;
}

int asm_write_hex(assembler_context_t *ctx, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return -1;
    }

    for (size_t i = 0; i < ctx->text_size; i++) {
        fprintf(f, "%02x", ctx->text_section[i]);
        if ((i + 1) % 16 == 0) fprintf(f, "\n");
        else fprintf(f, " ");
    }
    if (ctx->text_size % 16 != 0) fprintf(f, "\n");

    fclose(f);
    return 0;
}

int asm_write_debug_map(assembler_context_t *ctx, const char *filename) {
    char map_name[MAX_FILEPATH_LENGTH];
    FILE *f;

    if (!ctx || !filename) return -1;

    if (snprintf(map_name, sizeof(map_name), "%s.dbg", filename) >= (int)sizeof(map_name)) {
        fprintf(stderr, "Error: Debug map filename too long\n");
        return -1;
    }

    f = fopen(map_name, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot create debug map '%s'\n", map_name);
        return -1;
    }

    fprintf(f, "# x86_64-asm debug map (non-DWARF)\n");
    fprintf(f, "# base_address=0x%lx\n", ctx->base_address);
    fprintf(f, "# text_size=%zu data_size=%zu\n", ctx->text_size, ctx->data_size);
    fprintf(f, "\n");
    fprintf(f, "SYMBOLS\n");
    fprintf(f, "name,address,section,global,external\n");

    for (int i = 0; i < ctx->symbol_count; i++) {
        fprintf(f, "%s,0x%lx,%s,%d,%d\n",
                ctx->symbols[i].name,
                ctx->symbols[i].address,
                ctx->symbols[i].section == 0 ? "text" : "data",
                ctx->symbols[i].is_global ? 1 : 0,
                ctx->symbols[i].is_external ? 1 : 0);
    }

    fclose(f);
    return 0;
}

/* ============================================================================
 * DEBUG OUTPUT
 * ============================================================================ */

void asm_dump_symbols(assembler_context_t *ctx) {
    printf("Symbol Table:\n");
    printf("%-30s %-16s %s %s %s\n", "Name", "Address", "Global", "Extern", "Section");
    printf("%-30s %-16s %s %s %s\n", "----", "-------", "------", "------", "-------");

    for (int i = 0; i < ctx->symbol_count; i++) {
        printf("%-30s 0x%016lx %s      %s      %s\n",
               ctx->symbols[i].name,
               ctx->symbols[i].address,
               ctx->symbols[i].is_global ? "G" : " ",
               ctx->symbols[i].is_external ? "E" : " ",
               ctx->symbols[i].section == 0 ? "text" : "data");
    }
}

void asm_dump_output(assembler_context_t *ctx) {
    printf("\nGenerated Code (%zu bytes):\n", ctx->text_size);
    printf("Address    | Hex                                      | ASCII\n");
    printf("-----------|------------------------------------------|------\n");

    for (size_t i = 0; i < ctx->text_size; i += 16) {
        printf("0x%08lx | ", ctx->base_address + i);

        /* Hex */
        for (size_t j = 0; j < 16 && i + j < ctx->text_size; j++) {
            printf("%02x ", ctx->text_section[i + j]);
        }
        for (size_t j = ctx->text_size - i; j < 16; j++) {
            printf("   ");
        }

        printf("| ");

        /* ASCII */
        for (size_t j = 0; j < 16 && i + j < ctx->text_size; j++) {
            uint8_t c = ctx->text_section[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }
}
