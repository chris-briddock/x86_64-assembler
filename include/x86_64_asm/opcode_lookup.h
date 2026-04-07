#ifndef X86_64_ASM_OPCODE_LOOKUP_H
#define X86_64_ASM_OPCODE_LOOKUP_H

#include "x86_64_asm/x86_64_asm.h"

typedef struct {
    instruction_type_t inst;
    uint8_t reg_opcode;
    const char *mnemonic;
} x86_group1_entry_t;

typedef struct {
    uint8_t reg_opcode;
    const char *mnemonic;
} x86_group2_entry_t;

/*
 * Shared opcode metadata used by both encoder and disassembler.
 * This is the single source for group-opcode mappings so decode tables track
 * encoder instruction definitions.
 */
static const x86_group1_entry_t X86_GROUP1_TABLE[] = {
    {INST_ADD, 0, "add"},
    {INST_OR,  1, "or"},
    {INST_ADC, 2, "adc"},
    {INST_SBB, 3, "sbb"},
    {INST_AND, 4, "and"},
    {INST_SUB, 5, "sub"},
    {INST_XOR, 6, "xor"},
    {INST_CMP, 7, "cmp"}
};

static const x86_group2_entry_t X86_GROUP2_TABLE[] = {
    {0, "rol"},
    {1, "ror"},
    {4, "shl"},
    {5, "shr"},
    {7, "sar"}
};

static inline int x86_lookup_group1_reg_opcode(instruction_type_t inst, uint8_t *out_opcode) {
    size_t count = sizeof(X86_GROUP1_TABLE) / sizeof(X86_GROUP1_TABLE[0]);

    if (!out_opcode) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (X86_GROUP1_TABLE[i].inst == inst) {
            *out_opcode = X86_GROUP1_TABLE[i].reg_opcode;
            return 0;
        }
    }

    return -1;
}

static inline int x86_lookup_group1_mnemonic(uint8_t reg_opcode, const char **out_mnemonic) {
    size_t count = sizeof(X86_GROUP1_TABLE) / sizeof(X86_GROUP1_TABLE[0]);

    if (!out_mnemonic) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (X86_GROUP1_TABLE[i].reg_opcode == reg_opcode) {
            *out_mnemonic = X86_GROUP1_TABLE[i].mnemonic;
            return 0;
        }
    }

    return -1;
}

static inline int x86_lookup_group2_mnemonic(uint8_t reg_opcode, const char **out_mnemonic) {
    size_t count = sizeof(X86_GROUP2_TABLE) / sizeof(X86_GROUP2_TABLE[0]);

    if (!out_mnemonic) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (X86_GROUP2_TABLE[i].reg_opcode == reg_opcode) {
            *out_mnemonic = X86_GROUP2_TABLE[i].mnemonic;
            return 0;
        }
    }

    return -1;
}

#endif
