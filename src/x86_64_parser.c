/**
 * x86_64 Parser - Parse assembly syntax into instruction structures
 */

#include "x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

/* Global assembler context for macro processing during parsing */
static assembler_context_t *g_asm_ctx = NULL;

/* Token types */
typedef enum {
    TOK_NONE,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_COLON,      /* : */
    TOK_COMMA,      /* , */
    TOK_LBRACKET,   /* [ */
    TOK_RBRACKET,   /* ] */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR,       /* * */
    TOK_DOLLAR,     /* $ */
    TOK_NEWLINE,
    TOK_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    char text[MAX_LINE_LENGTH];
    int64_t value;      /* For numbers */
    int line;
    int column;
} token_t;

/* Parser state */
typedef struct {
    const char *input;
    const char *pos;
    const char *line_start;
    token_t current;
    token_t peek;
    int line;
} parser_state_t;

/* Instruction name table */
typedef struct {
    const char *name;
    instruction_type_t type;
} inst_name_t;

static const inst_name_t inst_table[] = {
    /* Data Movement */
    {"mov", INST_MOV},
    {"push", INST_PUSH},
    {"pop", INST_POP},
    {"lea", INST_LEA},
    {"movzx", INST_MOVZX},
    {"movsx", INST_MOVSX},
    {"xchg", INST_XCHG},
    {"bswap", INST_BSWAP},

    /* CMOVcc */
    {"cmova", INST_CMOVA}, {"cmovae", INST_CMOVAE},
    {"cmovb", INST_CMOVB}, {"cmovbe", INST_CMOVBE},
    {"cmove", INST_CMOVE}, {"cmovg", INST_CMOVG},
    {"cmovge", INST_CMOVGE}, {"cmovl", INST_CMOVL},
    {"cmovle", INST_CMOVLE}, {"cmovne", INST_CMOVNE},
    {"cmovno", INST_CMOVNO}, {"cmovnp", INST_CMOVNP},
    {"cmovns", INST_CMOVNS}, {"cmovo", INST_CMOVO},
    {"cmovp", INST_CMOVP}, {"cmovs", INST_CMOVS},

    /* Arithmetic */
    {"add", INST_ADD}, {"adc", INST_ADC},
    {"sub", INST_SUB}, {"sbb", INST_SBB},
    {"mul", INST_MUL}, {"imul", INST_IMUL},
    {"div", INST_DIV}, {"idiv", INST_IDIV},
    {"inc", INST_INC}, {"dec", INST_DEC},
    {"neg", INST_NEG}, {"cmp", INST_CMP},
    {"xor", INST_XOR}, {"or", INST_OR},
    {"and", INST_AND}, {"not", INST_NOT},
    {"test", INST_TEST},

    /* Bit operations */
    {"shl", INST_SHL}, {"shr", INST_SHR},
    {"sal", INST_SAL}, {"sar", INST_SAR},
    {"rol", INST_ROL}, {"ror", INST_ROR},
    {"rcl", INST_RCL}, {"rcr", INST_RCR},
    {"shld", INST_SHLD}, {"shrd", INST_SHRD},
    {"bt", INST_BT}, {"bts", INST_BTS},
    {"btr", INST_BTR}, {"btc", INST_BTC},
    {"bsf", INST_BSF}, {"bsr", INST_BSR},

    /* Control Flow */
    {"jmp", INST_JMP},
    {"ja", INST_JA}, {"jae", INST_JAE},
    {"jb", INST_JB}, {"jbe", INST_JBE},
    {"je", INST_JE}, {"jg", INST_JG},
    {"jge", INST_JGE}, {"jl", INST_JL},
    {"jle", INST_JLE}, {"jne", INST_JNE},
    {"jnz", INST_JNZ},
    {"jno", INST_JNO}, {"jnp", INST_JNP},
    {"jns", INST_JNS}, {"jo", INST_JO},
    {"jp", INST_JP}, {"js", INST_JS},
    {"call", INST_CALL}, {"ret", INST_RET},
    {"int", INST_INT},
    {"syscall", INST_SYSCALL}, {"sysret", INST_SYSRET},

    /* SETcc */
    {"seta", INST_SETA}, {"setae", INST_SETAE},
    {"setb", INST_SETB}, {"setbe", INST_SETBE},
    {"sete", INST_SETE}, {"setg", INST_SETG},
    {"setge", INST_SETGE}, {"setl", INST_SETL},
    {"setle", INST_SETLE}, {"setne", INST_SETNE},
    {"setno", INST_SETNO}, {"setnp", INST_SETNP},
    {"setns", INST_SETNS}, {"seto", INST_SETO},
    {"setp", INST_SETP}, {"sets", INST_SETS},

    /* Other */
    {"leave", INST_LEAVE},
    {"enter", INST_ENTER},
    {"nop", INST_NOP}, {"hlt", INST_HLT},
    {"clc", INST_CLC}, {"stc", INST_STC}, {"cmc", INST_CMC},
    {"cld", INST_CLD}, {"std", INST_STD},
    {"cli", INST_CLI}, {"sti", INST_STI},
    {"cbw", INST_CBW}, {"cwd", INST_CWD},
    {"cwde", INST_CWDE},
    {"cqo", INST_CQO}, {"cdq", INST_CDQ},
    {"cdqe", INST_CDQE},

    /* String instructions */
    {"movsb", INST_MOVSB}, {"movsw", INST_MOVSW},
    {"movsd", INST_MOVSD}, {"movsq", INST_MOVSQ},

    /* SSE Move Instructions */
    {"movaps", INST_MOVAPS}, {"movups", INST_MOVUPS},
    {"movss", INST_MOVSS},
    {"movapd", INST_MOVAPD}, {"movupd", INST_MOVUPD},
    /* Note: "movsd" is also a string instruction - disambiguated by operands */

    /* SSE Arithmetic - Add */
    {"addss", INST_ADDSS}, {"addps", INST_ADDPS},
    {"addsd", INST_ADDSD}, {"addpd", INST_ADDPD},

    /* SSE Arithmetic - Subtract */
    {"subss", INST_SUBSS}, {"subps", INST_SUBPS},
    {"subsd", INST_SUBSD}, {"subpd", INST_SUBPD},

    /* SSE Arithmetic - Multiply */
    {"mulss", INST_MULSS}, {"mulps", INST_MULPS},
    {"mulsd", INST_MULSD}, {"mulpd", INST_MULPD},

    /* SSE Arithmetic - Divide */
    {"divss", INST_DIVSS}, {"divps", INST_DIVPS},
    {"divsd", INST_DIVSD}, {"divpd", INST_DIVPD},

    /* SSE Packed Integer Moves */
    {"movdqa", INST_MOVDQA}, {"movdqu", INST_MOVDQU},

    /* SSE Packed Integer Arithmetic - Add */
    {"paddb", INST_PADDB}, {"paddw", INST_PADDW},
    {"paddd", INST_PADDD}, {"paddq", INST_PADDQ},

    /* SSE Packed Integer Arithmetic - Subtract */
    {"psubb", INST_PSUBB}, {"psubw", INST_PSUBW},
    {"psubd", INST_PSUBD}, {"psubq", INST_PSUBQ},

    /* SSE Packed Integer Compare */
    {"pcmpeqb", INST_PCMPEQB}, {"pcmpeqw", INST_PCMPEQW},
    {"pcmpeqd", INST_PCMPEQD}, {"pcmpeqq", INST_PCMPEQQ},
    {"pcmpgtb", INST_PCMPGTB}, {"pcmpgtw", INST_PCMPGTW},
    {"pcmpgtd", INST_PCMPGTD}, {"pcmpgtq", INST_PCMPGTQ},

    /* SSE Logical Operations */
    {"pand", INST_PAND}, {"pandn", INST_PANDNPD},
    {"por", INST_POR}, {"pxor", INST_PXOR},

    /* Pseudo-instructions */
    {"db", INST_DB}, {"dw", INST_DW},
    {"dd", INST_DD}, {"dq", INST_DQ},
    {"global", INST_GLOBAL}, {"extern", INST_EXTERN},
    {".global", INST_GLOBAL}, {".extern", INST_EXTERN},
    {"section", INST_SECTION}, {".section", INST_SECTION},
    {".text", INST_SECTION}, {".data", INST_SECTION},
    {"text", INST_SECTION}, {"data", INST_SECTION},

    {NULL, INST_UNKNOWN}
};

/* Initialize parser */
static void parser_init(parser_state_t *p, const char *input) {
    p->input = input;
    p->pos = input;
    p->line_start = input;
    p->line = 1;
    p->current.type = TOK_NONE;
    p->peek.type = TOK_NONE;
}

/* Print diagnostic error with source context */
static void parser_error_context(parser_state_t *p, const char *message) {
    /* Find end of current line */
    const char *line_end = p->pos;
    while (*line_end && *line_end != '\n') line_end++;
    
    /* Calculate line length and column */
    int line_len = (int)(line_end - p->line_start);
    int col = (int)(p->pos - p->line_start);
    
    /* Print error message */
    fprintf(stderr, "Error at line %d: %s\n", p->line, message);
    
    /* Print the source line */
    fprintf(stderr, "  ");
    for (int i = 0; i < line_len; i++) {
        fputc(p->line_start[i], stderr);
    }
    fprintf(stderr, "\n");
    
    /* Print column indicator */
    fprintf(stderr, "  ");
    for (int i = 0; i < col; i++) fputc(' ', stderr);
    fprintf(stderr, "^\n");
}

/* Skip whitespace and comments */
static void skip_whitespace(parser_state_t *p) {
    while (*p->pos) {
        if (*p->pos == ';' || (*p->pos == '/' && *(p->pos + 1) == '/')) {
            /* Skip to end of line but keep newline */
            while (*p->pos && *p->pos != '\n') p->pos++;
            return;  /* Stop at newline, let next_token handle it */
        } else if (*p->pos == '\n') {
            /* Don't consume newline - let next_token return it */
            return;
        } else if (isspace((unsigned char)*p->pos)) {
            p->pos++;
        } else {
            break;
        }
    }
}

/* Parse number */
static int64_t parse_number(const char *str, char **end) {
    int64_t val = 0;
    int base = 10;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        str += 2;
    } else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
        base = 2;
        str += 2;
    }

    char *e;
    val = strtoll(str, &e, base);
    if (end) *end = e;
    return val;
}

/* Get next token */
static token_t next_token(parser_state_t *p) {
    skip_whitespace(p);

    token_t tok;
    memset(&tok, 0, sizeof(tok));
    tok.line = p->line;
    tok.column = (int)(p->pos - p->line_start) + 1;

    if (*p->pos == '\0') {
        tok.type = TOK_EOF;
        return tok;
    }

    /* Newline - treat as significant token */
    if (*p->pos == '\n') {
        tok.type = TOK_NEWLINE;
        p->pos++;
        p->line++;
        p->line_start = p->pos;
        return tok;
    }

    /* Single character tokens */
    switch (*p->pos) {
        case ':': tok.type = TOK_COLON; p->pos++; return tok;
        case ',': tok.type = TOK_COMMA; p->pos++; return tok;
        case '[': tok.type = TOK_LBRACKET; p->pos++; return tok;
        case ']': tok.type = TOK_RBRACKET; p->pos++; return tok;
        case '+': tok.type = TOK_PLUS; p->pos++; return tok;
        case '-': tok.type = TOK_MINUS; p->pos++; return tok;
        case '*': tok.type = TOK_STAR; p->pos++; return tok;
        case '$': tok.type = TOK_DOLLAR; p->pos++; return tok;
    }

    /* String literal */
    if (*p->pos == '"' || *p->pos == '\'') {
        char quote = *p->pos++;
        int i = 0;
        while (*p->pos && *p->pos != quote && i < MAX_LINE_LENGTH - 1) {
            if (*p->pos == '\\' && *(p->pos + 1)) {
                p->pos++;
                switch (*p->pos) {
                    case 'n': tok.text[i++] = '\n'; break;
                    case 't': tok.text[i++] = '\t'; break;
                    case 'r': tok.text[i++] = '\r'; break;
                    case '0': tok.text[i++] = '\0'; break;
                    case '\\': tok.text[i++] = '\\'; break;
                    case '"': tok.text[i++] = '"'; break;
                    case '\'': tok.text[i++] = '\''; break;
                    default: tok.text[i++] = *p->pos; break;
                }
                p->pos++;
            } else {
                tok.text[i++] = *p->pos++;
            }
        }
        tok.text[i] = '\0';
        if (*p->pos == quote) p->pos++;
        tok.type = TOK_STRING;
        return tok;
    }

    /* Number */
    if (isdigit((unsigned char)*p->pos) ||
        (*p->pos == '0' && (*(p->pos + 1) == 'x' || *(p->pos + 1) == 'X' ||
                           *(p->pos + 1) == 'b' || *(p->pos + 1) == 'B'))) {
        const char *start = p->pos;
        if (*p->pos == '0' && (*(p->pos + 1) == 'x' || *(p->pos + 1) == 'X' ||
                              *(p->pos + 1) == 'b' || *(p->pos + 1) == 'B')) {
            p->pos += 2;
        }
        while (isxdigit((unsigned char)*p->pos)) p->pos++;

        int len = p->pos - start;
        if (len >= MAX_LINE_LENGTH) len = MAX_LINE_LENGTH - 1;
        strncpy(tok.text, start, len);
        tok.text[len] = '\0';
        tok.value = parse_number(tok.text, NULL);
        tok.type = TOK_NUMBER;
        return tok;
    }

    /* Identifier */
    if (isalpha((unsigned char)*p->pos) || *p->pos == '_' || *p->pos == '.' || *p->pos == '@') {
        int i = 0;
        while ((isalnum((unsigned char)*p->pos) || *p->pos == '_' || *p->pos == '.' || *p->pos == '@') &&
               i < MAX_LINE_LENGTH - 1) {
            tok.text[i++] = *p->pos++;
        }
        tok.text[i] = '\0';
        tok.type = TOK_IDENTIFIER;
        return tok;
    }

    /* Unknown character, skip it */
    p->pos++;
    tok.type = TOK_NONE;
    return tok;
}

/* Advance to next token */
static void advance(parser_state_t *p) {
    p->current = p->peek;
    if (p->peek.type == TOK_EOF) {
        return;
    }
    p->peek = next_token(p);
}

/* Initialize token stream */
static void init_tokens(parser_state_t *p) {
    p->peek = next_token(p);
    advance(p);
}

/* Expect token type */
static bool expect(parser_state_t *p, token_type_t type) {
    if (p->current.type == type) {
        advance(p);
        return true;
    }
    return false;
}

static void copy_bounded(char *dst, size_t dst_size, const char *src) {
    size_t n = 0;

    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }

    while (src[n] != '\0' && n + 1 < dst_size) {
        n++;
    }

    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* Parse register name */
static reg_t parse_reg(const char *name, reg_size_t *size) {
    return parse_register(name, size);
}

/* Parse memory operand [base+index*scale+disp] or [label] or [rip+label] */
static int parse_memory_operand(parser_state_t *p, mem_operand_t *mem) {
    memset(mem, 0, sizeof(*mem));
    mem->scale = 1;

    if (!expect(p, TOK_LBRACKET)) {
        parser_error_context(p, "Expected '[' for memory operand");
        return -1;
    }

    /* Parse components */
    while (p->current.type != TOK_RBRACKET && p->current.type != TOK_EOF) {
        if (p->current.type == TOK_IDENTIFIER) {
            reg_size_t size;
            reg_t reg = parse_reg(p->current.text, &size);

            if (reg != REG_NONE) {
                advance(p);

                /* Check for RIP register - for RIP-relative addressing */
                if (reg == RIP || (size == REG_SIZE_64 && strcasecmp(p->current.text, "rip") == 0)) {
                    mem->is_rip_relative = true;
                    /* Continue parsing - may have + label or displacement */
                    continue;
                }

                /* Check for scale factor */
                if (expect(p, TOK_STAR)) {
                    if (p->current.type == TOK_NUMBER) {
                        mem->index = reg;
                        mem->has_index = true;
                        mem->scale = (int)p->current.value;
                        advance(p);
                    } else {
                        parser_error_context(p, "Expected scale factor (1, 2, 4, or 8)");
                        return -1;
                    }
                } else {
                    /* This is base register (first one) or index */
                    if (!mem->has_base) {
                        mem->base = reg;
                        mem->has_base = true;
                    } else if (!mem->has_index) {
                        mem->index = reg;
                        mem->has_index = true;
                    } else {
                        parser_error_context(p, "Too many registers in memory operand (max: base + index)");
                        return -1;
                    }
                }
            } else {
                /* Label reference in memory - RIP-relative addressing */
                mem->is_rip_relative = true;
                copy_bounded(mem->label, sizeof(mem->label), p->current.text);
                advance(p);
            }
        } else if (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS) {
            int sign = 1;
            if (p->current.type == TOK_MINUS) {
                sign = -1;
            }
            advance(p);
            
            /* After +/- could be:
             * 1. A number (displacement)
             * 2. A register (index with optional scale)
             * 3. A label (RIP-relative)
             */
            if (p->current.type == TOK_NUMBER) {
                /* Displacement */
                mem->displacement += sign * (int32_t)p->current.value;
                mem->has_displacement = true;
                advance(p);
            } else if (p->current.type == TOK_IDENTIFIER) {
                /* Could be a register (index) or label */
                reg_size_t size;
                reg_t reg = parse_reg(p->current.text, &size);
                
                if (reg != REG_NONE) {
                    /* This is an index register */
                    advance(p);
                    
                    /* Check for scale factor */
                    if (expect(p, TOK_STAR)) {
                        if (p->current.type == TOK_NUMBER) {
                            mem->index = reg;
                            mem->has_index = true;
                            mem->scale = (int)p->current.value;
                            advance(p);
                        } else {
                            parser_error_context(p, "Expected scale factor (1, 2, 4, or 8)");
                            return -1;
                        }
                    } else {
                        /* No scale factor, scale = 1 */
                        mem->index = reg;
                        mem->has_index = true;
                        mem->scale = 1;
                    }
                } else {
                    /* Label reference - RIP-relative addressing with displacement */
                    if (sign == -1) {
                        parser_error_context(p, "Negative displacement with label reference not supported");
                        return -1;
                    }
                    mem->is_rip_relative = true;
                    copy_bounded(mem->label, sizeof(mem->label), p->current.text);
                    advance(p);
                }
            } else {
                parser_error_context(p, "Expected register or number after +/- in memory operand");
                return -1;
            }
        } else if (p->current.type == TOK_STAR) {
            /* Scale factor after index without explicit base+ prefix */
            advance(p);
            if (p->current.type == TOK_NUMBER) {
                if (mem->has_index) {
                    mem->scale = (int)p->current.value;
                } else {
                    parser_error_context(p, "Scale factor requires an index register");
                    return -1;
                }
                advance(p);
            } else {
                parser_error_context(p, "Expected scale factor (1, 2, 4, or 8)");
                return -1;
            }
        } else {
            parser_error_context(p, "Unexpected token in memory operand");
            return -1;
        }
    }

    if (!expect(p, TOK_RBRACKET)) {
        parser_error_context(p, "Expected ']' to close memory operand");
        return -1;
    }

    return 0;
}

/* Parse single operand */
static int parse_operand(parser_state_t *p, operand_t *op) {
    memset(op, 0, sizeof(*op));

    /* Immediate */
    if (p->current.type == TOK_DOLLAR) {
        advance(p);
        if (p->current.type == TOK_NUMBER) {
            op->type = OPERAND_IMM;
            op->immediate = p->current.value;
            advance(p);
        } else if (p->current.type == TOK_IDENTIFIER) {
            /* $label - address of label */
            op->type = OPERAND_LABEL;
            copy_bounded(op->label, sizeof(op->label), p->current.text);
            advance(p);
        } else {
            parser_error_context(p, "Expected immediate value after '$'");
            return -1;
        }
        return 0;
    }

    /* Memory reference */
    if (p->current.type == TOK_LBRACKET) {
        op->type = OPERAND_MEM;
        return parse_memory_operand(p, &op->mem);
    }

    /* Register */
    if (p->current.type == TOK_IDENTIFIER) {
        reg_size_t size;
        reg_t reg = parse_reg(p->current.text, &size);

        if (reg != REG_NONE) {
            op->type = OPERAND_REG;
            op->reg = reg;
            op->reg_size = size;
            advance(p);
            return 0;
        }

        /* Label */
        op->type = OPERAND_LABEL;
        copy_bounded(op->label, sizeof(op->label), p->current.text);
        advance(p);
        return 0;
    }

    /* Numeric immediate without $ (usually for jumps/calls) */
    if (p->current.type == TOK_NUMBER) {
        op->type = OPERAND_IMM;
        op->immediate = p->current.value;
        advance(p);
        return 0;
    }

    fprintf(stderr, "Error: Unexpected token '%s' at line %d\n", p->current.text, p->line);
    parser_error_context(p, "Invalid operand syntax");
    return -1;
}

static bool is_sse_move_inst(instruction_type_t type) {
    return type == INST_MOVAPS || type == INST_MOVUPS || type == INST_MOVSS ||
           type == INST_MOVAPD || type == INST_MOVUPD || type == INST_MOVSD_XMM ||
           type == INST_MOVDQA || type == INST_MOVDQU;
}

static bool is_sse_arith_inst(instruction_type_t type) {
    return (type >= INST_ADDSS && type <= INST_DIVPD);
}

static bool is_sse_packed_inst(instruction_type_t type) {
    return (type >= INST_PADDB && type <= INST_PXOR);
}

static bool is_xmm_reg_operand(const operand_t *op) {
    return op->type == OPERAND_REG && op->reg_size == REG_SIZE_XMM;
}

static int validate_sse_operands(const parsed_instruction_t *inst, int line) {
    if (is_sse_move_inst(inst->type)) {
        if (inst->operand_count != 2) {
            fprintf(stderr, "Error: SSE move requires 2 operands at line %d\n", line);
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool dst_xmm = is_xmm_reg_operand(dst);
        bool src_xmm = is_xmm_reg_operand(src);
        bool dst_mem = (dst->type == OPERAND_MEM);
        bool src_mem = (src->type == OPERAND_MEM);

        if (dst_mem && src_mem) {
            fprintf(stderr, "Error: SSE move cannot have two memory operands at line %d\n", line);
            return -1;
        }
        if (dst_mem && !src_xmm) {
            fprintf(stderr, "Error: SSE move memory destination requires XMM source at line %d\n", line);
            return -1;
        }
        if (dst_xmm && !src_xmm && !src_mem) {
            fprintf(stderr, "Error: SSE move XMM destination requires XMM or memory source at line %d\n", line);
            return -1;
        }
        if (!dst_xmm && !src_xmm) {
            fprintf(stderr, "Error: SSE move requires at least one XMM operand at line %d\n", line);
            return -1;
        }
    }

    if (is_sse_arith_inst(inst->type)) {
        if (inst->operand_count != 2) {
            fprintf(stderr, "Error: SSE arithmetic requires 2 operands at line %d\n", line);
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool src_xmm = is_xmm_reg_operand(src);
        bool src_mem = (src->type == OPERAND_MEM);

        if (!is_xmm_reg_operand(dst)) {
            fprintf(stderr, "Error: SSE arithmetic destination must be XMM register at line %d\n", line);
            return -1;
        }
        if (!src_xmm && !src_mem) {
            fprintf(stderr, "Error: SSE arithmetic source must be XMM or memory at line %d\n", line);
            return -1;
        }
    }

    if (is_sse_packed_inst(inst->type)) {
        if (inst->operand_count != 2) {
            fprintf(stderr, "Error: SSE packed instruction requires 2 operands at line %d\n", line);
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool src_xmm = is_xmm_reg_operand(src);
        bool src_mem = (src->type == OPERAND_MEM);

        if (!is_xmm_reg_operand(dst)) {
            fprintf(stderr, "Error: SSE packed instruction destination must be XMM register at line %d\n", line);
            return -1;
        }
        if (!src_xmm && !src_mem) {
            fprintf(stderr, "Error: SSE packed instruction source must be XMM or memory at line %d\n", line);
            return -1;
        }
    }

    return 0;
}

/* Parse instruction */
static int parse_instruction(parser_state_t *p, parsed_instruction_t *inst) {
    memset(inst, 0, sizeof(*inst));
    inst->line_number = p->line;
    inst->label_column = 1;


    if (p->current.type == TOK_EOF) {
        inst->type = INST_NOP;
        return 0;
    }

    /* Check for label */
    if (p->current.type == TOK_IDENTIFIER && p->peek.type == TOK_COLON) {
        /* Label definition */
        inst->has_label = true;
        copy_bounded(inst->label, sizeof(inst->label), p->current.text);
        inst->label_column = p->current.column;
        advance(p); /* identifier */
        advance(p); /* colon */
    }

    /* Parse instruction mnemonic */
    if (p->current.type == TOK_IDENTIFIER) {
        inst->type = INST_UNKNOWN;

        /* Look up instruction */
        for (int i = 0; inst_table[i].name != NULL; i++) {
            if (strcasecmp(p->current.text, inst_table[i].name) == 0) {
                inst->type = inst_table[i].type;
                break;
            }
        }

        if (inst->type == INST_UNKNOWN) {
            parser_error_context(p, "Unknown instruction");
            return -1;
        }

        advance(p);
    } else if (inst->has_label) {
        /* Label-only line */
        inst->type = INST_NOP;
        return 0;
    } else {
        /* Empty line or comment */
        inst->type = INST_NOP;
        return 0;
    }

    /* Parse operands */
    while (p->current.type != TOK_NEWLINE && p->current.type != TOK_EOF) {
        if (inst->operand_count >= MAX_OPERANDS) {
            parser_error_context(p, "Too many operands");
            return -1;
        }

        if (parse_operand(p, &inst->operands[inst->operand_count]) < 0) {
            return -1;
        }
        inst->operand_count++;

        if (p->current.type == TOK_COMMA) {
            advance(p);
        }
    }

    /* Disambiguate MOVSD: string instruction (no operands) vs SSE move (XMM operands) */
    if (inst->type == INST_MOVSD) {
        if (inst->operand_count > 0) {
            /* Has operands - must be SSE MOVSD */
            inst->type = INST_MOVSD_XMM;
        }
        /* No operands - keep as string instruction INST_MOVSD */
    }

    if (validate_sse_operands(inst, p->line) < 0) {
        return -1;
    }

    return 0;
}

/* Main parse function (internal) */
parsed_instruction_t *parse_source_internal(const char *source, int *count) {
    parser_state_t p;
    parser_init(&p, source);
    init_tokens(&p);

    /* First pass: count instructions */
    int capacity = 256;
    int n = 0;
    parsed_instruction_t *insts = malloc(capacity * sizeof(parsed_instruction_t));
    if (!insts) return NULL;

    while (p.current.type != TOK_EOF) {
        if (n >= capacity) {
            capacity *= 2;
            parsed_instruction_t *new_insts = realloc(insts, capacity * sizeof(parsed_instruction_t));
            if (!new_insts) {
                free(insts);
                return NULL;
            }
            insts = new_insts;
        }

        if (parse_instruction(&p, &insts[n]) == 0) {
            if (insts[n].type != INST_NOP || insts[n].has_label) {
                n++;
            }
        } else {
            free(insts);
            return NULL;
        }

        /* After parse_instruction, skip NEWLINE if present */
        if (p.current.type == TOK_NEWLINE) {
            advance(&p);
        }
    }

    *count = n;
    return insts;
}

/* Main parse function with macro preprocessing */
parsed_instruction_t *parse_source(const char *source, int *count) {
    /* Simple case: no assembler context, parse directly */
    if (!g_asm_ctx) {
        return parse_source_internal(source, count);
    }
    
    /* Preprocess macros */
    char *preprocessed = preprocess_macros(g_asm_ctx, source);
    if (!preprocessed) {
        fprintf(stderr, "Error: Macro preprocessing failed\n");
        return NULL;
    }
    
    /* Parse the preprocessed source */
    parsed_instruction_t *insts = parse_source_internal(preprocessed, count);
    free(preprocessed);
    
    return insts;
}

/* Parse source with assembler context for macro support */
parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count) {
    g_asm_ctx = ctx;
    parsed_instruction_t *insts = parse_source(source, count);
    g_asm_ctx = NULL;
    return insts;
}

/* Free parsed instructions */
void free_instructions(parsed_instruction_t *insts) {
    free(insts);
}

/* ============================================================================
 * MACRO SUPPORT IMPLEMENTATION
 * ============================================================================ */

/* Look up a macro by name */
macro_t *macro_lookup(assembler_context_t *ctx, const char *name) {
    if (!ctx || !name) return NULL;
    
    for (int i = 0; i < ctx->macro_count; i++) {
        if (ctx->macros[i].defined &&
            strcasecmp(ctx->macros[i].name, name) == 0) {
            return &ctx->macros[i];
        }
    }
    return NULL;
}

/* Check if a name is a defined macro */
bool is_macro_name(assembler_context_t *ctx, const char *name) {
    return macro_lookup(ctx, name) != NULL;
}

/* Get the parameter count for a macro */
int get_macro_arg_count(assembler_context_t *ctx, const char *name) {
    macro_t *macro = macro_lookup(ctx, name);
    if (macro) {
        return macro->param_count;
    }
    return -1;
}

/* Start defining a new macro */
int macro_define(assembler_context_t *ctx, const char *name) {
    if (!ctx || !name) return -1;
    
    if (ctx->macro_count >= MAX_MACROS) {
        fprintf(stderr, "Error: Too many macros defined (max %d)\n", MAX_MACROS);
        return -1;
    }
    
    /* Check for duplicate macro names */
    if (macro_lookup(ctx, name) != NULL) {
        fprintf(stderr, "Error: Macro '%s' already defined\n", name);
        return -1;
    }
    
    macro_t *macro = &ctx->macros[ctx->macro_count];
    memset(macro, 0, sizeof(*macro));
    strncpy(macro->name, name, MAX_LABEL_LENGTH - 1);
    macro->name[MAX_LABEL_LENGTH - 1] = '\0';
    macro->defined = true;
    
    ctx->in_macro_definition = true;
    ctx->current_macro = macro;
    
    return 0;
}

/* Add a parameter to the current macro being defined */
void macro_add_param(assembler_context_t *ctx, const char *param_name) {
    if (!ctx || !ctx->current_macro || !param_name) return;
    
    macro_t *macro = ctx->current_macro;
    if (macro->param_count >= MAX_MACRO_PARAMS) {
        fprintf(stderr, "Error: Too many parameters for macro '%s' (max %d)\n",
                macro->name, MAX_MACRO_PARAMS);
        return;
    }
    
    strncpy(macro->params[macro->param_count].name, param_name, MAX_LABEL_LENGTH - 1);
    macro->params[macro->param_count].name[MAX_LABEL_LENGTH - 1] = '\0';
    macro->param_count++;
}

/* Add a line to the macro body */
void macro_add_body_line(assembler_context_t *ctx, const char *line) {
    if (!ctx || !ctx->current_macro || !line) return;
    
    macro_t *macro = ctx->current_macro;
    if (macro->body_line_count >= MAX_MACRO_BODY_LINES) {
        fprintf(stderr, "Error: Macro '%s' body too long (max %d lines)\n",
                macro->name, MAX_MACRO_BODY_LINES);
        return;
    }
    
    strncpy(macro->body[macro->body_line_count], line, MAX_LINE_LENGTH - 1);
    macro->body[macro->body_line_count][MAX_LINE_LENGTH - 1] = '\0';
    macro->body_line_count++;
}

/* End macro definition */
void macro_end_definition(assembler_context_t *ctx) {
    if (!ctx || !ctx->current_macro) return;
    
    ctx->macro_count++;
    ctx->in_macro_definition = false;
    ctx->current_macro = NULL;
}

/* Substitute parameters in a macro body line */
static void substitute_params(const char *line, const char **args, int arg_count,
                              const macro_t *macro, int local_counter,
                              char *output, size_t output_size) {
    if (!line || !output || output_size == 0) return;
    
    const char *src = line;
    char *dst = output;
    size_t remaining = output_size - 1;
    
    while (*src && remaining > 0) {
        /* Check for parameter reference: \param or $param */
        if ((*src == '\\' || *src == '$') && *(src + 1)) {
            const char *param_start = src + 1;
            const char *param_end = param_start;
            
            /* Find end of parameter name (alphanumeric + underscore) */
            while (*param_end && (isalnum((unsigned char)*param_end) || *param_end == '_')) {
                param_end++;
            }
            
            size_t param_len = param_end - param_start;
            if (param_len > 0) {
                /* Check for local label suffix \@ */
                if (*src == '\\' && *param_start == '@' && param_len == 1) {
                    /* Local label counter substitution */
                    char counter_str[32];
                    snprintf(counter_str, sizeof(counter_str), "%d", local_counter);
                    size_t counter_len = strlen(counter_str);
                    if (counter_len <= remaining) {
                        memcpy(dst, counter_str, counter_len);
                        dst += counter_len;
                        remaining -= counter_len;
                    }
                    src = param_end;
                    continue;
                }
                
                /* Look up parameter by name */
                for (int i = 0; i < macro->param_count; i++) {
                    if (strlen(macro->params[i].name) == param_len &&
                        strncmp(macro->params[i].name, param_start, param_len) == 0) {
                        /* Found matching parameter */
                        if (i < arg_count && args[i]) {
                            size_t arg_len = strlen(args[i]);
                            if (arg_len <= remaining) {
                                memcpy(dst, args[i], arg_len);
                                dst += arg_len;
                                remaining -= arg_len;
                            }
                        }
                        src = param_end;
                        goto next_char;
                    }
                }
            }
        }
        
        /* Regular character - copy as-is */
        *dst++ = *src++;
        remaining--;
        
    next_char:
        continue;
    }
    
    *dst = '\0';
}

/* Expand a macro invocation into expanded lines */
static char *expand_macro(assembler_context_t *ctx, const char *macro_name,
                          const char **args, int arg_count, int *line_count) {
    if (!ctx || !macro_name || !line_count) return NULL;

    macro_t *macro = macro_lookup(ctx, macro_name);
    if (!macro) {
        fprintf(stderr, "Error: Macro '%s' not found\n", macro_name);
        return NULL;
    }

    /* Check recursion depth */
    if (ctx->macro_ctx.expansion_depth >= MAX_MACRO_EXPANSION_DEPTH) {
        fprintf(stderr, "Error: Macro expansion depth exceeded for '%s' (max %d)\n",
                macro_name, MAX_MACRO_EXPANSION_DEPTH);
        return NULL;
    }
    
    /* Increment expansion depth */
    ctx->macro_ctx.expansion_depth++;
    ctx->macro_ctx.local_label_counter++;
    int local_counter = ctx->macro_ctx.local_label_counter;
    copy_bounded(ctx->macro_ctx.macro_name, sizeof(ctx->macro_ctx.macro_name), macro_name);
    
    /* Allocate buffer for expanded macro */
    size_t buf_size = macro->body_line_count * MAX_LINE_LENGTH * 2;
    char *expanded = malloc(buf_size);
    if (!expanded) {
        ctx->macro_ctx.expansion_depth--;
        return NULL;
    }
    
    expanded[0] = '\0';
    size_t pos = 0;
    
    /* Expand each line of the macro body */
    for (int i = 0; i < macro->body_line_count; i++) {
        char line_buf[MAX_LINE_LENGTH * 2];
        substitute_params(macro->body[i], args, arg_count, macro, local_counter,
                          line_buf, sizeof(line_buf));
        
        size_t line_len = strlen(line_buf);
        if (pos + line_len + 2 < buf_size) {
            memcpy(expanded + pos, line_buf, line_len);
            pos += line_len;
            expanded[pos++] = '\n';
            expanded[pos] = '\0';
        }
    }
    
    *line_count = macro->body_line_count;
    
    /* Decrement expansion depth */
    ctx->macro_ctx.expansion_depth--;
    
    return expanded;
}

/* Parse .macro directive from a line */
static int parse_macro_directive(const char *line, char *macro_name,
                                  char params[][MAX_LABEL_LENGTH], int *param_count) {
    if (!line || !macro_name) return -1;
    
    /* Skip leading whitespace */
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Check for .macro or macro */
    if (strncasecmp(p, ".macro", 6) != 0 && strncasecmp(p, "macro", 5) != 0) {
        return -1;
    }
    
    /* Skip directive */
    if (*p == '.') p += 6;
    else p += 5;
    
    /* Skip whitespace */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Parse macro name */
    if (!*p) {
        fprintf(stderr, "Error: .macro requires a name\n");
        return -1;
    }
    
    int i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '.') && i < MAX_LABEL_LENGTH - 1) {
        macro_name[i++] = *p++;
    }
    macro_name[i] = '\0';
    
    if (i == 0) {
        fprintf(stderr, "Error: Invalid macro name\n");
        return -1;
    }
    
    *param_count = 0;
    
    /* Skip whitespace */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Parse parameters if present */
    while (*p && *p != ';' && *p != '/' && *param_count < MAX_MACRO_PARAMS) {
        /* Skip optional comma */
        if (*p == ',') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
        }
        
        /* Parse parameter name */
        i = 0;
        while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < MAX_LABEL_LENGTH - 1) {
            params[*param_count][i++] = *p++;
        }
        
        if (i > 0) {
            params[*param_count][i] = '\0';
            (*param_count)++;
        }
        
        /* Skip whitespace and optional comma */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ',') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
        }
    }
    
    return 0;
}

/* Check if a line is .endm directive */
static bool is_endm_directive(const char *line) {
    if (!line) return false;
    
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    
    return (strncasecmp(p, ".endm", 5) == 0 || strncasecmp(p, "endm", 4) == 0);
}

/* Check if a line is a macro invocation and extract arguments */
static bool parse_macro_invocation(assembler_context_t *ctx, const char *line,
                                    char *macro_name, char args[][MAX_LINE_LENGTH],
                                    int *arg_count) {
    if (!ctx || !line || !macro_name) return false;
    
    *arg_count = 0;
    
    /* Skip leading whitespace */
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Check for label (skip it if present) */
    const char *colon = strchr(p, ':');
    if (colon) {
        p = colon + 1;
        while (*p && isspace((unsigned char)*p)) p++;
    }
    
    /* Parse potential macro name */
    if (!*p || !(isalpha((unsigned char)*p) || *p == '_')) {
        return false;
    }
    
    int i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '.') && i < MAX_LABEL_LENGTH - 1) {
        macro_name[i++] = *p++;
    }
    macro_name[i] = '\0';
    
    /* Check if it's a defined macro */
    if (!is_macro_name(ctx, macro_name)) {
        return false;
    }
    
    /* Skip whitespace */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Parse arguments */
    while (*p && *p != ';' && *p != '/' && *arg_count < MAX_MACRO_PARAMS) {
        /* Skip optional comma */
        if (*p == ',') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
        }
        
        /* Skip leading whitespace for this argument */
        while (*p && isspace((unsigned char)*p)) p++;
        
        /* Parse argument (could be register, immediate, memory, label) */
        i = 0;
        int paren_depth = 0;
        int bracket_depth = 0;
        
        while (*p && (*p != ',' || paren_depth > 0 || bracket_depth > 0) &&
               *p != ';' && !(p[0] == '/' && p[1] == '/') &&
               i < MAX_LINE_LENGTH - 1) {
            if (*p == '(') paren_depth++;
            else if (*p == ')') paren_depth--;
            else if (*p == '[') bracket_depth++;
            else if (*p == ']') bracket_depth--;
            
            args[*arg_count][i++] = *p++;
        }
        
        if (i > 0) {
            /* Trim trailing whitespace */
            while (i > 0 && isspace((unsigned char)args[*arg_count][i-1])) {
                i--;
            }
            args[*arg_count][i] = '\0';
            (*arg_count)++;
        }
        
        /* Skip whitespace and optional comma */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ',') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
        }
    }
    
    return true;
}

static int append_text(char **buffer, size_t *capacity, size_t *len, const char *text, size_t text_len) {
    if (*len + text_len + 1 > *capacity) {
        size_t new_capacity = *capacity;
        while (*len + text_len + 1 > new_capacity) {
            new_capacity *= 2;
        }
        char *new_buffer = realloc(*buffer, new_capacity);
        if (!new_buffer) return -1;
        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    memcpy(*buffer + *len, text, text_len);
    *len += text_len;
    (*buffer)[*len] = '\0';
    return 0;
}

static void dirname_from_path(const char *path, char *out_dir) {
    const char *slash;

    if (!path || !path[0]) {
        strcpy(out_dir, ".");
        return;
    }

    slash = strrchr(path, '/');
    if (!slash) {
        strcpy(out_dir, ".");
        return;
    }

    if (slash == path) {
        strcpy(out_dir, "/");
        return;
    }

    size_t dir_len = (size_t)(slash - path);
    if (dir_len >= MAX_FILEPATH_LENGTH) {
        dir_len = MAX_FILEPATH_LENGTH - 1;
    }

    memcpy(out_dir, path, dir_len);
    out_dir[dir_len] = '\0';
}

static int parse_include_filename(const char *line, char *filename, size_t filename_size) {
    const char *p;
    char quote;
    size_t i = 0;

    if (!line || !filename || filename_size == 0) return 0;

    p = line;
    while (*p && isspace((unsigned char)*p)) p++;

    if (strncasecmp(p, ".include", 8) != 0 && strncasecmp(p, "include", 7) != 0) {
        return 0;
    }

    p += (*p == '.') ? 8 : 7;
    while (*p && isspace((unsigned char)*p)) p++;

    if (*p != '"' && *p != '\'') {
        fprintf(stderr, "Error: .include directive requires a quoted filename\n");
        return -1;
    }

    quote = *p++;
    while (*p && *p != quote && i < filename_size - 1) {
        filename[i++] = *p++;
    }
    filename[i] = '\0';

    if (*p != quote) {
        fprintf(stderr, "Error: Unterminated filename in .include directive\n");
        return -1;
    }

    if (i == 0) {
        fprintf(stderr, "Error: Empty filename in .include directive\n");
        return -1;
    }

    return 1;
}

static char *expand_includes_recursive(assembler_context_t *ctx, const char *source,
                                       const char *base_dir, int depth) {
    const char *p;
    char line[MAX_LINE_LENGTH];
    char *output;
    size_t output_capacity;
    size_t output_len;

    if (!source) return NULL;

    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: Include depth exceeded (max %d)\n", MAX_INCLUDE_DEPTH);
        return NULL;
    }

    output_capacity = strlen(source) + 128;
    output = malloc(output_capacity);
    if (!output) return NULL;
    output[0] = '\0';
    output_len = 0;

    p = source;
    while (*p) {
        int i = 0;
        bool had_newline = false;
        char include_name[MAX_FILEPATH_LENGTH];
        int include_parse;

        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *p++;
        }
        if (*p == '\n') {
            had_newline = true;
            p++;
        }
        line[i] = '\0';

        include_parse = parse_include_filename(line, include_name, sizeof(include_name));
        if (include_parse < 0) {
            free(output);
            return NULL;
        }

        if (include_parse == 1) {
            char resolved_path[MAX_FILEPATH_LENGTH];
            char child_base_dir[MAX_FILEPATH_LENGTH];
            char *included_source;
            char *expanded_child;

            if (include_name[0] == '/') {
                strncpy(resolved_path, include_name, sizeof(resolved_path) - 1);
                resolved_path[sizeof(resolved_path) - 1] = '\0';
            } else {
                if (!base_dir || !base_dir[0] || strcmp(base_dir, ".") == 0) {
                    if (snprintf(resolved_path, sizeof(resolved_path), "%s", include_name) >= (int)sizeof(resolved_path)) {
                        fprintf(stderr, "Error: Include path too long: %s\n", include_name);
                        free(output);
                        return NULL;
                    }
                } else {
                    if (snprintf(resolved_path, sizeof(resolved_path), "%s/%s", base_dir, include_name) >= (int)sizeof(resolved_path)) {
                        fprintf(stderr, "Error: Include path too long: %s/%s\n", base_dir, include_name);
                        free(output);
                        return NULL;
                    }
                }
            }

            if (ctx && is_circular_include(ctx, resolved_path)) {
                fprintf(stderr, "Error: Circular include detected: '%s'\n", resolved_path);
                free(output);
                return NULL;
            }

            if (ctx && ctx->include_ctx.tracker.count < MAX_INCLUDE_DEPTH) {
                snprintf(ctx->include_ctx.tracker.filenames[ctx->include_ctx.tracker.count],
                         MAX_FILEPATH_LENGTH, "%s", resolved_path);
                ctx->include_ctx.tracker.count++;
            }

            included_source = read_file_contents(resolved_path);
            if (!included_source) {
                if (ctx && ctx->include_ctx.tracker.count > 0) ctx->include_ctx.tracker.count--;
                free(output);
                return NULL;
            }

            dirname_from_path(resolved_path, child_base_dir);
            expanded_child = expand_includes_recursive(ctx, included_source, child_base_dir, depth + 1);
            free(included_source);

            if (ctx && ctx->include_ctx.tracker.count > 0) ctx->include_ctx.tracker.count--;

            if (!expanded_child) {
                free(output);
                return NULL;
            }

            if (append_text(&output, &output_capacity, &output_len, expanded_child, strlen(expanded_child)) < 0) {
                free(expanded_child);
                free(output);
                return NULL;
            }
            free(expanded_child);
            continue;
        }

        if (append_text(&output, &output_capacity, &output_len, line, strlen(line)) < 0) {
            free(output);
            return NULL;
        }
        if (had_newline) {
            if (append_text(&output, &output_capacity, &output_len, "\n", 1) < 0) {
                free(output);
                return NULL;
            }
        }
    }

    return output;
}

/* Preprocess source to handle macro definitions and expansions */
char *preprocess_macros(assembler_context_t *ctx, const char *source) {
    if (!ctx || !source) return NULL;

    char base_dir[MAX_FILEPATH_LENGTH];
    char *expanded_source = NULL;
    const char *source_text = source;

    dirname_from_path(ctx->current_filename, base_dir);
    expanded_source = expand_includes_recursive(ctx, source, base_dir, 0);
    if (!expanded_source) {
        return NULL;
    }
    source_text = expanded_source;
    
    /* Initialize macro context */
    ctx->macro_count = 0;
    ctx->in_macro_definition = false;
    ctx->current_macro = NULL;
    ctx->macro_ctx.expansion_depth = 0;
    ctx->macro_ctx.local_label_counter = 0;
    ctx->macro_ctx.macro_name[0] = '\0';
    
    /* First pass: collect macro definitions */
    const char *p = source_text;
    char line[MAX_LINE_LENGTH];
    
    while (*p) {
        /* Read a line */
        int i = 0;
        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;
        
        /* Check for .macro directive */
        char macro_name[MAX_LABEL_LENGTH];
        char params[MAX_MACRO_PARAMS][MAX_LABEL_LENGTH];
        int param_count = 0;
        
        if (parse_macro_directive(line, macro_name, params, &param_count) == 0) {
            /* Start macro definition */
            if (macro_define(ctx, macro_name) < 0) {
                return NULL;
            }
            
            /* Add parameters */
            for (int i = 0; i < param_count; i++) {
                macro_add_param(ctx, params[i]);
            }
            
            /* Read macro body until .endm */
            while (*p) {
                i = 0;
                while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
                    line[i++] = *p++;
                }
                line[i] = '\0';
                if (*p == '\n') {
                    line[i++] = '\n';
                    line[i] = '\0';
                    p++;
                }
                
                /* Check for .endm */
                if (is_endm_directive(line)) {
                    macro_end_definition(ctx);
                    break;
                }
                
                /* Add line to macro body (remove trailing newline) */
                if (i > 0 && line[i-1] == '\n') {
                    line[i-1] = '\0';
                }
                macro_add_body_line(ctx, line);
            }
        }
    }
    
    /* Second pass: expand macro invocations and process includes */
    size_t output_size = strlen(source_text) * 4 + 1;  /* Allow for expansion */
    char *output = malloc(output_size);
    if (!output) {
        free(expanded_source);
        return NULL;
    }
    
    size_t out_pos = 0;
    p = source_text;
    
    while (*p) {
        /* Read a line */
        int i = 0;
        const char *line_start = p;
        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;
        
        /* Skip macro definitions (don't include in output) */
        char test_name[MAX_LABEL_LENGTH];
        char test_params[MAX_MACRO_PARAMS][MAX_LABEL_LENGTH];
        int test_count = 0;
        if (parse_macro_directive(line, test_name, test_params, &test_count) == 0) {
            /* Skip until .endm */
            while (*p) {
                i = 0;
                while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
                    line[i++] = *p++;
                }
                line[i] = '\0';
                if (*p == '\n') p++;
                
                if (is_endm_directive(line)) {
                    break;
                }
            }
            continue;
        }
        
        /* Check for macro invocation */
        char macro_name[MAX_LABEL_LENGTH];
        char args[MAX_MACRO_PARAMS][MAX_LINE_LENGTH];
        int arg_count = 0;
        
        if (parse_macro_invocation(ctx, line, macro_name, args, &arg_count)) {
            /* Expand macro */
            const char *arg_ptrs[MAX_MACRO_PARAMS];
            for (int j = 0; j < arg_count; j++) {
                arg_ptrs[j] = args[j];
            }
            
            int expanded_lines = 0;
            char *expanded = expand_macro(ctx, macro_name, arg_ptrs, arg_count, &expanded_lines);
            
            if (expanded) {
                size_t exp_len = strlen(expanded);
                if (out_pos + exp_len < output_size - 1) {
                    memcpy(output + out_pos, expanded, exp_len);
                    out_pos += exp_len;
                }
                free(expanded);
            }
        } else {
            /* Copy line as-is */
            size_t line_len = p - line_start;
            if (out_pos + line_len < output_size - 1) {
                memcpy(output + out_pos, line_start, line_len);
                out_pos += line_len;
            }
        }
    }
    
    output[out_pos] = '\0';
    free(expanded_source);
    return output;
}

/* ============================================================================
 * INCLUDE FILE SUPPORT IMPLEMENTATION
 * ============================================================================ */

/* Read entire file contents into a malloc'd buffer */
char *read_file_contents(const char *filename) {
    if (!filename) return NULL;
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open include file '%s': %s\n", filename, strerror(errno));
        return NULL;
    }
    
    /* Get file size */
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    
    /* Allocate buffer with extra space for null terminator */
    char *content = malloc(size + 2);
    if (!content) {
        fclose(f);
        return NULL;
    }
    
    /* Read file */
    size_t read = fread(content, 1, size, f);
    fclose(f);
    
    if (read != (size_t)size) {
        free(content);
        return NULL;
    }
    
    /* Ensure null termination and trailing newline */
    content[size] = '\n';
    content[size + 1] = '\0';
    
    return content;
}

/* Check if a file is already in the include chain (circular include detection) */
bool is_circular_include(assembler_context_t *ctx, const char *filename) {
    if (!ctx || !filename) return false;
    
    for (int i = 0; i < ctx->include_ctx.tracker.count; i++) {
        if (strcmp(ctx->include_ctx.tracker.filenames[i], filename) == 0) {
            return true;
        }
    }
    return false;
}

/* Push a new file onto the include stack */
int include_push(assembler_context_t *ctx, const char *filename, const char *content) {
    if (!ctx || !filename || !content) return -1;
    
    /* Check depth limit */
    if (ctx->include_ctx.depth >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: Include depth exceeded (max %d) when including '%s'\n",
                MAX_INCLUDE_DEPTH, filename);
        return -1;
    }
    
    /* Check for circular includes */
    if (is_circular_include(ctx, filename)) {
        fprintf(stderr, "Error: Circular include detected: '%s' is already being processed\n", filename);
        return -1;
    }
    
    /* Save current file state if there is one */
    if (ctx->include_ctx.depth > 0) {
        file_entry_t *current = &ctx->include_ctx.stack[ctx->include_ctx.depth - 1];
        current->source_pos = ctx->source_pos;
        current->line_number = ctx->line_number;
    }
    
    /* Push new file */
    file_entry_t *entry = &ctx->include_ctx.stack[ctx->include_ctx.depth];
    memset(entry, 0, sizeof(*entry));
    
    strncpy(entry->filename, filename, MAX_FILEPATH_LENGTH - 1);
    entry->filename[MAX_FILEPATH_LENGTH - 1] = '\0';
    entry->source = (char *)content;
    entry->source_pos = content;
    entry->line_number = 1;
    
    /* Update context */
    ctx->source = content;
    ctx->source_pos = content;
    ctx->line_number = 1;
    strncpy(ctx->current_filename, filename, MAX_FILEPATH_LENGTH - 1);
    ctx->current_filename[MAX_FILEPATH_LENGTH - 1] = '\0';
    
    /* Add to tracker for circular detection */
    if (ctx->include_ctx.tracker.count < MAX_INCLUDE_DEPTH) {
        strncpy(ctx->include_ctx.tracker.filenames[ctx->include_ctx.tracker.count],
                filename, MAX_FILEPATH_LENGTH - 1);
        ctx->include_ctx.tracker.filenames[ctx->include_ctx.tracker.count][MAX_FILEPATH_LENGTH - 1] = '\0';
        ctx->include_ctx.tracker.count++;
    }
    
    ctx->include_ctx.depth++;
    
    return 0;
}

/* Pop a file from the include stack, return true if more files remain */
int include_pop(assembler_context_t *ctx) {
    if (!ctx) return -1;
    
    if (ctx->include_ctx.depth <= 0) {
        return -1;  /* Nothing to pop */
    }
    
    /* Get current entry */
    file_entry_t *entry = &ctx->include_ctx.stack[ctx->include_ctx.depth - 1];
    
    /* Free the source buffer */
    if (entry->source) {
        free(entry->source);
        entry->source = NULL;
    }
    
    /* Remove from tracker */
    if (ctx->include_ctx.tracker.count > 0) {
        ctx->include_ctx.tracker.count--;
        ctx->include_ctx.tracker.filenames[ctx->include_ctx.tracker.count][0] = '\0';
    }
    
    ctx->include_ctx.depth--;
    
    /* Restore previous file context if any */
    if (ctx->include_ctx.depth > 0) {
        file_entry_t *prev = &ctx->include_ctx.stack[ctx->include_ctx.depth - 1];
        ctx->source = prev->source;
        ctx->source_pos = prev->source_pos;
        ctx->line_number = prev->line_number;
        strncpy(ctx->current_filename, prev->filename, MAX_FILEPATH_LENGTH - 1);
        ctx->current_filename[MAX_FILEPATH_LENGTH - 1] = '\0';
    }
    
    return ctx->include_ctx.depth;
}

/* Include a file - read and push onto stack */
int include_file(assembler_context_t *ctx, const char *filename) {
    if (!ctx || !filename) return -1;
    
    /* Read file content */
    char *content = read_file_contents(filename);
    if (!content) {
        return -1;
    }
    
    /* Push onto stack */
    if (include_push(ctx, filename, content) < 0) {
        free(content);
        return -1;
    }
    
    return 0;
}

/* Get next character from source with include file support */
int source_getc(assembler_context_t *ctx) {
    if (!ctx) return EOF;
    
    while (ctx->include_ctx.depth > 0 || ctx->source_pos) {
        /* If we have a source position, try to read from it */
        if (ctx->source_pos && *ctx->source_pos) {
            char c = *ctx->source_pos;
            ctx->source_pos++;
            if (c == '\n') {
                ctx->line_number++;
            }
            return (unsigned char)c;
        }
        
        /* End of current file - pop to previous */
        if (ctx->include_ctx.depth > 0) {
            include_pop(ctx);
        } else {
            break;
        }
    }
    
    return EOF;
}

/* Get current source location for error reporting */
void get_source_location(assembler_context_t *ctx, char *filename, int *line_number) {
    if (!ctx) {
        if (filename) filename[0] = '\0';
        if (line_number) *line_number = 0;
        return;
    }
    
    if (filename) {
        strncpy(filename, ctx->current_filename, MAX_FILEPATH_LENGTH - 1);
        filename[MAX_FILEPATH_LENGTH - 1] = '\0';
    }
    
    if (line_number) {
        *line_number = ctx->line_number;
    }
}

