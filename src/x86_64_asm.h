/* Compatibility shim: use the canonical public header in include/x86_64_asm/. */

#ifndef X86_64_ASM_COMPAT_H
#define X86_64_ASM_COMPAT_H

#include "x86_64_asm/x86_64_asm.h"

/* Internal output/encoding helpers (not part of the public API). */
int emit_byte(assembler_context_t *ctx, uint8_t byte);
int emit_bytes(assembler_context_t *ctx, const uint8_t *bytes, size_t count);
int emit_word(assembler_context_t *ctx, uint16_t word);
int emit_dword(assembler_context_t *ctx, uint32_t dword);
int emit_qword(assembler_context_t *ctx, uint64_t qword);
int emit_le16(assembler_context_t *ctx, int16_t val);
int emit_le32(assembler_context_t *ctx, int32_t val);
int emit_le64(assembler_context_t *ctx, int64_t val);
int emit_rex(assembler_context_t *ctx, bool w, const operand_t *reg_op,
			 const operand_t *rm_op, const operand_t *sib_index_op);
int emit_modrm_sib(assembler_context_t *ctx, uint8_t reg_opcode,
				   const operand_t *operand, reg_size_t size);

void listing_start_instruction(assembler_context_t *ctx, int line_number, const char *source);
void listing_emit_byte(assembler_context_t *ctx, uint8_t byte);
void listing_end_instruction(assembler_context_t *ctx);

int encode_instruction(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_mov(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_unary_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_lea(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_push_pop(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_jmp(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_jcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_call_ret(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_int(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_syscall(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sysret(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_nop(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_hlt(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_cwd_cdq_cqo(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_cbw_cwde_cdqe(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_enter(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_leave(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_clc_stc_cmc(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_cld_std(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_cli_sti(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_setcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_cmov(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_bswap(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_xchg(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_imul(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_div_idiv_mul(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_test(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_not(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_shift(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_string(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_bit_manip(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_shld_shrd(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_bit_scan(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_mov(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_arith(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_arith(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_cmp(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_logic(assembler_context_t *ctx, const parsed_instruction_t *inst);

int fits_in_int8(int32_t val);
uint8_t make_modrm(uint8_t mod, uint8_t reg, uint8_t rm);
uint8_t make_sib(uint8_t scale, uint8_t index, uint8_t base);
int needs_rex(const operand_t *op);
uint8_t make_rex(bool w, bool r, bool x, bool b);
reg_t parse_register(const char *name, reg_size_t *size);
const char *register_name(reg_t reg, reg_size_t size);
int get_reg_size_bytes(reg_size_t size);

/* Internal macro/parser/include helpers (not part of public API). */
macro_t *macro_lookup(assembler_context_t *ctx, const char *name);
int macro_define(assembler_context_t *ctx, const char *name);
void macro_add_param(assembler_context_t *ctx, const char *param_name);
void macro_add_body_line(assembler_context_t *ctx, const char *line);
void macro_end_definition(assembler_context_t *ctx);
char *macro_expand_line(assembler_context_t *ctx, const char *line, const char **args, int arg_count);
bool is_macro_name(assembler_context_t *ctx, const char *name);
int get_macro_arg_count(assembler_context_t *ctx, const char *name);
char *preprocess_macros(assembler_context_t *ctx, const char *source);

typedef struct {
	uint64_t parse_calls;
	uint64_t parse_ns;
	uint64_t parse_instruction_calls;
	uint64_t parse_instruction_ns;
	uint64_t next_token_calls;
	uint64_t next_token_ns;
	uint64_t realloc_events;
	uint64_t peak_instruction_capacity;
} parser_profile_stats_t;

parsed_instruction_t *parse_source(const char *source, int *count);
void free_instructions(parsed_instruction_t *insts);
parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count);
void parser_profile_enable(assembler_context_t *ctx, bool enabled);
void parser_profile_reset(assembler_context_t *ctx);
void parser_profile_get(const assembler_context_t *ctx, parser_profile_stats_t *out_stats);

int include_file(assembler_context_t *ctx, const char *filename);
int include_push(assembler_context_t *ctx, const char *filename, char *content);
int include_pop(assembler_context_t *ctx);
bool is_circular_include(const assembler_context_t *ctx, const char *filename);
char *read_file_contents(const char *filename);
int source_getc(assembler_context_t *ctx);
void get_source_location(const assembler_context_t *ctx, char *filename, int *line_number);

#endif /* X86_64_ASM_COMPAT_H */
