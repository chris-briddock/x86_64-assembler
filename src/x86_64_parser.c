/**
 * x86_64 Parser - Parse assembly syntax into instruction structures
 */

#include "x86_64_asm/x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

/* Token types */
typedef enum {
    TOK_NONE,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR,       /* 'A' - character literal */
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

static bool g_parser_profile_enabled = false;
static parser_profile_stats_t g_parser_profile_stats;

static token_t next_token(parser_state_t *p);
static int parse_instruction(parser_state_t *p, parsed_instruction_t *inst);

static uint64_t parser_now_ns(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static size_t estimate_instruction_capacity(const char *source) {
    size_t line_count = 1;
    const char *p = source;

    if (!source || source[0] == '\0') {
        return 64u;
    }

    while (*p) {
        if (*p == '\n') {
            line_count++;
        }
        p++;
    }

    /* Slightly over-allocate to avoid growth churn on dense instruction files. */
    if (line_count < 256u) {
        return 256u;
    }
    return line_count + (line_count / 8u) + 32u;
}

static token_t profiled_next_token(parser_state_t *p) {
    uint64_t start_ns = 0;
    token_t tok;

    if (g_parser_profile_enabled) {
        start_ns = parser_now_ns();
    }

    tok = next_token(p);

    if (g_parser_profile_enabled) {
        g_parser_profile_stats.next_token_calls++;
        g_parser_profile_stats.next_token_ns += (parser_now_ns() - start_ns);
    }

    return tok;
}

/**
 * Validate that a string contains valid UTF-8 encoding
 * Returns: number of valid UTF-8 code points, or -1 if invalid
 */
static int utf8_expected_len(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return -1;
}

static int utf8_has_valid_continuations(const unsigned char *s, int expected_len) {
    for (int i = 1; i < expected_len; i++) {
        if (s[i] == 0 || (s[i] & 0xC0) != 0x80) {
            return 0;
        }
    }
    return 1;
}

static int validate_utf8(const char *str) {
    int count = 0;
    const unsigned char *s = (const unsigned char *)str;

    while (*s) {
        int expected_len = utf8_expected_len(*s);
        if (expected_len < 0) {
            return -1;
        }

        if (expected_len > 1 && !utf8_has_valid_continuations(s, expected_len)) {
            return -1;
        }

        count++;
        s += expected_len;
    }

    return count;
}

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
    {"resb", INST_RESB}, {"resw", INST_RESW},
    {"resd", INST_RESD}, {"resq", INST_RESQ},
    {"equ", INST_EQU},
    {"times", INST_TIMES},
    {"align", INST_ALIGN},
    {"incbin", INST_INCBIN},
    {"comm", INST_COMM},
    {".comm", INST_COMM},
    {"lcomm", INST_LCOMM},
    {".lcomm", INST_LCOMM},
    {"global", INST_GLOBAL}, {"extern", INST_EXTERN},
    {".global", INST_GLOBAL}, {".extern", INST_EXTERN},
    {"weak", INST_WEAK}, {"hidden", INST_HIDDEN},
    {".weak", INST_WEAK}, {".hidden", INST_HIDDEN},
    {"section", INST_SECTION}, {".section", INST_SECTION},
    {"segment", INST_SECTION}, {".segment", INST_SECTION},
    {".text", INST_SECTION}, {".data", INST_SECTION},
    {"text", INST_SECTION}, {"data", INST_SECTION},
    {".bss", INST_SECTION}, {"bss", INST_SECTION},
    {".rodata", INST_SECTION}, {"rodata", INST_SECTION},

    {NULL, INST_UNKNOWN}
};

static int min3_int(int a, int b, int c) {
    int m = a < b ? a : b;
    return m < c ? m : c;
}

/* Case-insensitive Levenshtein distance used for did-you-mean diagnostics. */
static int instruction_name_distance(const char *lhs, const char *rhs) {
    size_t lhs_len = strlen(lhs);
    size_t rhs_len = strlen(rhs);

    if (lhs_len > 63 || rhs_len > 63) {
        return 999;
    }

    int prev[64];
    int curr[64];

    for (size_t j = 0; j <= rhs_len; j++) {
        prev[j] = (int)j;
    }

    for (size_t i = 1; i <= lhs_len; i++) {
        curr[0] = (int)i;
        char lc = (char)tolower((unsigned char)lhs[i - 1]);

        for (size_t j = 1; j <= rhs_len; j++) {
            char rc = (char)tolower((unsigned char)rhs[j - 1]);
            int cost = (lc == rc) ? 0 : 1;
            curr[j] = min3_int(
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            );
        }

        for (size_t j = 0; j <= rhs_len; j++) {
            prev[j] = curr[j];
        }
    }

    return prev[rhs_len];
}

static const char *find_closest_instruction_name(const char *name, int *out_distance) {
    const char *best_name = NULL;
    int best_dist = 999;

    for (int i = 0; inst_table[i].name != NULL; i++) {
        int dist = instruction_name_distance(name, inst_table[i].name);
        if (dist < best_dist) {
            best_dist = dist;
            best_name = inst_table[i].name;
        }
    }

    if (out_distance) {
        *out_distance = best_dist;
    }
    return best_name;
}

/* Initialize parser */
static void parser_init(parser_state_t *p, const char *input) {
    p->input = input;
    p->pos = input;
    p->line_start = input;
    p->line = 1;
    p->current.type = TOK_NONE;
    p->peek.type = TOK_NONE;
}

/* Print diagnostic error with source context and remediation hint. */
static void parser_error_context_ex(const parser_state_t *p, const char *category,
                                    const char *message, const char *suggestion) {
    /* Find end of current line */
    const char *line_end = p->pos;
    while (*line_end && *line_end != '\n') line_end++;
    
    /* Calculate line length and column */
    int line_len = (int)(line_end - p->line_start);
    int col = (int)(p->pos - p->line_start) + 1;
    
    /* Print error message */
        fprintf(stderr, "Error at line %d, column %d: [%s] %s\n",
            p->line, col, category, message);
    
    /* Print the source line */
    fprintf(stderr, "  ");
    for (int i = 0; i < line_len; i++) {
        fputc(p->line_start[i], stderr);
    }
    fprintf(stderr, "\n");
    
    /* Print column indicator */
    fprintf(stderr, "  ");
    for (int i = 1; i < col; i++) fputc(' ', stderr);
    fprintf(stderr, "^\n");

    if (suggestion && suggestion[0] != '\0') {
        fprintf(stderr, "\nSuggestion: %s\n", suggestion);
    }
}

/* Backward-compatible wrapper for existing parser call sites. */
static void parser_error_context(const parser_state_t *p, const char *message) {
    parser_error_context_ex(
        p,
        "Syntax",
        message,
        "Check instruction syntax and operands against the assembly reference."
    );
}

/* Emit standardized diagnostics for parser paths that do not have parser_state_t context. */
static void parser_diag_ctx(const assembler_context_t *ctx, const char *category,
                            const char *message, const char *suggestion) {
    int line = 1;
    if (ctx && ctx->line_number > 0) {
        line = ctx->line_number;
    }

    fprintf(stderr, "Error at line %d, column 1: [%s] %s\n", line, category, message);
    if (suggestion && suggestion[0] != '\0') {
        fprintf(stderr, "\nSuggestion: %s\n", suggestion);
    }
}

static void capture_original_line(const parser_state_t *p, char *out, size_t out_size) {
    const char *line_end;
    size_t len;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';

    if (!p || !p->line_start) {
        return;
    }

    line_end = p->line_start;
    while (*line_end && *line_end != '\n' && *line_end != '\r') {
        line_end++;
    }

    len = (size_t)(line_end - p->line_start);
    if (len >= out_size) {
        len = out_size - 1;
    }

    memcpy(out, p->line_start, len);
    out[len] = '\0';
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
        default:
            break;
    }

    /* Character literal (single-quoted) */
    if (*p->pos == '\'') {
        p->pos++;  /* Skip opening quote */
        char ch = 0;
        
        if (*p->pos == '\\' && *(p->pos + 1)) {
            /* Escape sequence */
            p->pos++;
            switch (*p->pos) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '0': ch = '\0'; break;
                case '\\': ch = '\\'; break;
                case '"': ch = '"'; break;
                case '\'': ch = '\''; break;
                default: ch = *p->pos; break;
            }
            p->pos++;
        } else if (*p->pos && *p->pos != '\'') {
            /* Regular character */
            ch = *p->pos++;
        }
        
        /* Skip closing quote */
        if (*p->pos == '\'') p->pos++;
        
        tok.type = TOK_CHAR;
        tok.value = (int64_t)(unsigned char)ch;
        tok.text[0] = ch;
        tok.text[1] = '\0';
        return tok;
    }
    
    /* String literal (double-quoted) */
    if (*p->pos == '"') {
        p->pos++;  /* Skip opening quote */
        int i = 0;
        while (*p->pos && *p->pos != '"' && i < MAX_LINE_LENGTH - 1) {
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
        if (*p->pos == '"') p->pos++;
        tok.type = TOK_STRING;

        /* Validate UTF-8 encoding */
        if (validate_utf8(tok.text) < 0) {
            fprintf(stderr, "Warning: Invalid UTF-8 sequence in string at line %d, column %d\n",
                    tok.line, tok.column);
        }
        
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

        size_t len = (size_t)(p->pos - start);
        if (len >= MAX_LINE_LENGTH) len = (size_t)(MAX_LINE_LENGTH - 1);
        memcpy(tok.text, start, len);
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
    p->peek = profiled_next_token(p);
}

/* Initialize token stream */
static void init_tokens(parser_state_t *p) {
    p->peek = profiled_next_token(p);
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

static bool is_valid_scale_factor(int64_t scale) {
    return scale == 1 || scale == 2 || scale == 4 || scale == 8;
}

static mem_segment_t parse_segment_override_name(const char *name) {
    if (!name) return MEM_SEG_DEFAULT;
    if (strcasecmp(name, "fs") == 0) return MEM_SEG_FS;
    if (strcasecmp(name, "gs") == 0) return MEM_SEG_GS;
    return MEM_SEG_DEFAULT;
}

/* Parse memory operand [base+index*scale+disp] or [label] or [rip+label] */
static int parse_memory_operand(parser_state_t *p, mem_operand_t *mem) {
    memset(mem, 0, sizeof(*mem));
    mem->scale = 1;
    mem->segment_override = MEM_SEG_DEFAULT;

    if (!expect(p, TOK_LBRACKET)) {
        parser_error_context_ex(
            p,
            "Syntax",
            "Expected '[' for memory operand",
            "Use memory syntax like [rax], [rbx+8], or [base+index*scale+disp]."
        );
        return -1;
    }

    /* Explicit absolute addressing form: [abs 0x1234] */
    if (p->current.type == TOK_IDENTIFIER && strcasecmp(p->current.text, "abs") == 0) {
        advance(p);

        if (p->current.type != TOK_NUMBER) {
            parser_error_context_ex(
                p,
                "Syntax",
                "[abs ...] requires a numeric address",
                "Use forms like [abs 0x1234] or [abs 4096]."
            );
            return -1;
        }

        mem->is_absolute = true;
        mem->displacement = (int32_t)p->current.value;
        mem->has_displacement = true;
        advance(p);

        if (!expect(p, TOK_RBRACKET)) {
            parser_error_context_ex(
                p,
                "Syntax",
                "Expected ']' to close memory operand",
                "Close memory operands with ']'."
            );
            return -1;
        }

        return 0;
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
                        if (!is_valid_scale_factor(p->current.value)) {
                            parser_error_context_ex(
                                p,
                                "Syntax",
                                "Expected scale factor (1, 2, 4, or 8)",
                                "Valid scale factors are 1, 2, 4, and 8."
                            );
                            return -1;
                        }
                        mem->index = reg;
                        mem->has_index = true;
                        mem->scale = (int)p->current.value;
                        advance(p);
                    } else {
                        parser_error_context_ex(
                            p,
                            "Syntax",
                            "Expected scale factor (1, 2, 4, or 8)",
                            "Put a numeric scale after '*', for example rcx*4."
                        );
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
                        parser_error_context_ex(
                            p,
                            "Syntax",
                            "Too many registers in memory operand (max: base + index)",
                            "Use at most one base and one index register per memory operand."
                        );
                        return -1;
                    }
                }
            } else {
                /* Bare [label] defaults to absolute displacement form.
                 * Explicit RIP-relative remains available via [rip + label]. */
                copy_bounded(mem->label, sizeof(mem->label), p->current.text);
                advance(p);
            }
        } else if (p->current.type == TOK_NUMBER) {
            mem->displacement += (int32_t)p->current.value;
            mem->has_displacement = true;
            advance(p);
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
                            if (!is_valid_scale_factor(p->current.value)) {
                                parser_error_context_ex(
                                    p,
                                    "Syntax",
                                    "Expected scale factor (1, 2, 4, or 8)",
                                    "Valid scale factors are 1, 2, 4, and 8."
                                );
                                return -1;
                            }
                            mem->index = reg;
                            mem->has_index = true;
                            mem->scale = (int)p->current.value;
                            advance(p);
                        } else {
                            parser_error_context_ex(
                                p,
                                "Syntax",
                                "Expected scale factor (1, 2, 4, or 8)",
                                "Put a numeric scale after '*', for example r13*2."
                            );
                            return -1;
                        }
                    } else {
                        /* No scale factor, scale = 1 */
                        mem->index = reg;
                        mem->has_index = true;
                        mem->scale = 1;
                    }
                } else {
                    /* Label reference with optional displacement.
                     * Use RIP-relative only when caller explicitly wrote [rip+label]. */
                    if (sign == -1 && mem->is_rip_relative) {
                        parser_error_context_ex(
                            p,
                            "Constraint",
                            "Negative displacement with label reference not supported",
                            "Rewrite as [label + positive_disp] or materialize the address in a register first."
                        );
                        return -1;
                    }
                    copy_bounded(mem->label, sizeof(mem->label), p->current.text);
                    advance(p);
                }
            } else {
                parser_error_context_ex(
                    p,
                    "Syntax",
                    "Expected register or number after +/- in memory operand",
                    "After '+' or '-', use a register, numeric displacement, or label."
                );
                return -1;
            }
        } else if (p->current.type == TOK_STAR) {
            /* Scale factor after index without explicit base+ prefix */
            advance(p);
            if (p->current.type == TOK_NUMBER) {
                if (mem->has_index) {
                    if (!is_valid_scale_factor(p->current.value)) {
                        parser_error_context_ex(
                            p,
                            "Syntax",
                            "Expected scale factor (1, 2, 4, or 8)",
                            "Valid scale factors are 1, 2, 4, and 8."
                        );
                        return -1;
                    }
                    mem->scale = (int)p->current.value;
                } else {
                    parser_error_context_ex(
                        p,
                        "Constraint",
                        "Scale factor requires an index register",
                        "Specify an index register before '*', for example [rbx + rcx*4]."
                    );
                    return -1;
                }
                advance(p);
            } else {
                parser_error_context_ex(
                    p,
                    "Syntax",
                    "Expected scale factor (1, 2, 4, or 8)",
                    "Put a numeric scale after '*', for example *2 or *8."
                );
                return -1;
            }
        } else {
            parser_error_context_ex(
                p,
                "Syntax",
                "Unexpected token in memory operand",
                "Use memory operands like [base + index*scale + displacement]."
            );
            return -1;
        }
    }

    if (!expect(p, TOK_RBRACKET)) {
        parser_error_context_ex(
            p,
            "Syntax",
            "Expected ']' to close memory operand",
            "Close memory operands with ']'."
        );
        return -1;
    }

    if (mem->is_rip_relative && (mem->has_base || mem->has_index)) {
        parser_error_context_ex(
            p,
            "Constraint",
            "RIP-relative addressing cannot combine base/index registers",
            "Use [rip + disp_or_label] only, without explicit base/index registers."
        );
        return -1;
    }

    return 0;
}

/* Parse single operand */
static int parse_operand(parser_state_t *p, operand_t *op) {
    memset(op, 0, sizeof(*op));

    /* Segment override form: fs:[...], gs:[...] */
    if (p->current.type == TOK_IDENTIFIER && p->peek.type == TOK_COLON) {
        mem_segment_t seg = parse_segment_override_name(p->current.text);
        if (seg != MEM_SEG_DEFAULT) {
            advance(p); /* fs/gs */
            advance(p); /* : */

            if (p->current.type != TOK_LBRACKET) {
                parser_error_context_ex(
                    p,
                    "Syntax",
                    "Segment override requires a memory operand",
                    "Use segment overrides only with memory forms like fs:[rax] or gs:[rbx+8]."
                );
                return -1;
            }

            op->type = OPERAND_MEM;
            if (parse_memory_operand(p, &op->mem) < 0) {
                return -1;
            }
            op->mem.segment_override = seg;
            return 0;
        }
    }

    /* Immediate */
    if (p->current.type == TOK_DOLLAR) {
        advance(p);
        if (p->current.type == TOK_NUMBER) {
            op->type = OPERAND_IMM;
            op->immediate = p->current.value;
            advance(p);
        } else if (p->current.type == TOK_IDENTIFIER) {
            char lhs[MAX_LABEL_LENGTH];
            copy_bounded(lhs, sizeof(lhs), p->current.text);
            advance(p);

            if (p->current.type == TOK_MINUS && p->peek.type == TOK_IDENTIFIER) {
                op->type = OPERAND_LABEL_DIFF;
                copy_bounded(op->label, sizeof(op->label), lhs);
                advance(p); /* '-' */
                copy_bounded(op->label_rhs, sizeof(op->label_rhs), p->current.text);
                advance(p); /* rhs label */
            } else {
                /* $label - address of label */
                op->type = OPERAND_LABEL;
                copy_bounded(op->label, sizeof(op->label), lhs);
            }
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

        /* Label or label arithmetic expression */
        char lhs[MAX_LABEL_LENGTH];
        copy_bounded(lhs, sizeof(lhs), p->current.text);
        advance(p);

        if (p->current.type == TOK_MINUS && p->peek.type == TOK_IDENTIFIER) {
            op->type = OPERAND_LABEL_DIFF;
            copy_bounded(op->label, sizeof(op->label), lhs);
            advance(p); /* '-' */
            copy_bounded(op->label_rhs, sizeof(op->label_rhs), p->current.text);
            advance(p); /* rhs label */
        } else {
            op->type = OPERAND_LABEL;
            copy_bounded(op->label, sizeof(op->label), lhs);
        }
        return 0;
    }

    /* Numeric immediate without $ (usually for jumps/calls) */
    if (p->current.type == TOK_NUMBER) {
        op->type = OPERAND_IMM;
        op->immediate = p->current.value;
        advance(p);
        return 0;
    }

    /* Character literal (e.g., 'A', '\n') */
    if (p->current.type == TOK_CHAR) {
        op->type = OPERAND_IMM;
        op->immediate = p->current.value;
        advance(p);
        return 0;
    }

    /* String literal (for db directive) */
    if (p->current.type == TOK_STRING) {
        op->type = OPERAND_STRING;
        copy_bounded(op->string, sizeof(op->string), p->current.text);
        advance(p);
        
        /* Handle string concatenation - merge adjacent string literals */
        while (p->current.type == TOK_STRING) {
            size_t current_len = strlen(op->string);
            size_t to_add = strlen(p->current.text);
            if (current_len + to_add < sizeof(op->string) - 1) {
                strncat(op->string, p->current.text, sizeof(op->string) - current_len - 1);
            }
            advance(p);
        }
        return 0;
    }

    char message[256];
    snprintf(message, sizeof(message), "Unexpected token '%.200s' in operand", p->current.text);
    parser_error_context_ex(
        p,
        "Syntax",
        message,
        "Use a register, immediate, label, or memory operand in this position"
    );
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

static int validate_sse_operands(const parser_state_t *p, const parsed_instruction_t *inst) {
    if (is_sse_move_inst(inst->type)) {
        if (inst->operand_count != 2) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE move requires exactly 2 operands",
                "Use the form: movaps/movups/movdqa/movdqu xmmN, xmmM|[mem] or [mem], xmmN"
            );
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool dst_xmm = is_xmm_reg_operand(dst);
        bool src_xmm = is_xmm_reg_operand(src);
        bool dst_mem = (dst->type == OPERAND_MEM);
        bool src_mem = (src->type == OPERAND_MEM);

        if (dst_mem && src_mem) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE move does not allow memory-to-memory operands",
                "Use an XMM register as either the source or destination operand"
            );
            return -1;
        }
        if (dst_mem && !src_xmm) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE move memory destination requires XMM source register",
                "Use: movaps/movups/movdqa/movdqu [mem], xmmN"
            );
            return -1;
        }
        if (dst_xmm && !src_xmm && !src_mem) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE move with XMM destination requires XMM or memory source",
                "Use: movaps/movups/movdqa/movdqu xmmN, xmmM or xmmN, [mem]"
            );
            return -1;
        }
        if (!dst_xmm && !src_xmm) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE move requires at least one XMM operand",
                "Use an XMM register in either the source or destination operand"
            );
            return -1;
        }
    }

    if (is_sse_arith_inst(inst->type)) {
        if (inst->operand_count != 2) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE arithmetic requires exactly 2 operands",
                "Use the form: addps/subps/mulps/divps xmmN, xmmM|[mem]"
            );
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool src_xmm = is_xmm_reg_operand(src);
        bool src_mem = (src->type == OPERAND_MEM);

        if (!is_xmm_reg_operand(dst)) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE arithmetic destination must be XMM register",
                "Use an XMM register as the first operand"
            );
            return -1;
        }
        if (!src_xmm && !src_mem) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE arithmetic source must be XMM or memory",
                "Use an XMM register or [base+index*scale+disp] as the second operand"
            );
            return -1;
        }
    }

    if (is_sse_packed_inst(inst->type)) {
        if (inst->operand_count != 2) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE packed instruction requires exactly 2 operands",
                "Use the form: padd*/psub*/pcmpeq*/pcmpgt*/pand*/por/pxor xmmN, xmmM|[mem]"
            );
            return -1;
        }

        const operand_t *dst = &inst->operands[0];
        const operand_t *src = &inst->operands[1];
        bool src_xmm = is_xmm_reg_operand(src);
        bool src_mem = (src->type == OPERAND_MEM);

        if (!is_xmm_reg_operand(dst)) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE packed instruction destination must be an XMM register",
                "Use an XMM register as the first operand"
            );
            return -1;
        }
        if (!src_xmm && !src_mem) {
            parser_error_context_ex(
                p,
                "Constraint",
                "SSE packed instruction source must be an XMM register or memory operand",
                "Use an XMM register or [base+index*scale+disp] as the second operand"
            );
            return -1;
        }
    }

    return 0;
}

static instruction_type_t lookup_instruction_type(const char *mnemonic) {
    for (int i = 0; inst_table[i].name != NULL; i++) {
        if (strcasecmp(mnemonic, inst_table[i].name) == 0) {
            return inst_table[i].type;
        }
    }
    return INST_UNKNOWN;
}

static int maybe_parse_equ_directive(parser_state_t *p, parsed_instruction_t *inst) {
    if (!(p->current.type == TOK_IDENTIFIER && p->peek.type == TOK_IDENTIFIER &&
          strcasecmp(p->peek.text, "equ") == 0)) {
        return 0;
    }

    inst->has_label = true;
    copy_bounded(inst->label, sizeof(inst->label), p->current.text);
    inst->label_column = p->current.column;
    inst->type = INST_EQU;
    advance(p);
    advance(p);

    if (inst->operand_count >= MAX_OPERANDS) {
        parser_error_context(p, "Too many operands");
        return -1;
    }
    if (parse_operand(p, &inst->operands[inst->operand_count]) < 0) {
        return -1;
    }
    if (inst->operands[inst->operand_count].type != OPERAND_IMM &&
        inst->operands[inst->operand_count].type != OPERAND_LABEL &&
        inst->operands[inst->operand_count].type != OPERAND_LABEL_DIFF) {
        parser_error_context(p, "equ directive requires a numeric value, label, or label subtraction");
        return -1;
    }
    inst->operand_count++;
    return 1;
}

static int maybe_apply_section_shorthand(const parser_state_t *p, parsed_instruction_t *inst,
                                         const char *mnemonic_text) {
    bool section_keyword =
        strcasecmp(mnemonic_text, "section") == 0 ||
        strcasecmp(mnemonic_text, ".section") == 0 ||
        strcasecmp(mnemonic_text, "segment") == 0 ||
        strcasecmp(mnemonic_text, ".segment") == 0;

    if (inst->type != INST_SECTION || section_keyword) {
        return 0;
    }

    inst->operands[0].type = OPERAND_LABEL;
    copy_bounded(inst->operands[0].label, sizeof(inst->operands[0].label), mnemonic_text);
    inst->operand_count = 1;
    if (p->current.type != TOK_NEWLINE && p->current.type != TOK_EOF) {
        parser_error_context(p, "Section shorthand directives do not take operands");
        return -1;
    }
    return 1;
}

static int parse_times_count(const char **pp, int *out_count) {
    const int max_times_count = 10000;
    const char *cursor = *pp;
    int count = 0;

    if (*cursor == '$') {
        cursor++;
    }

    if (*cursor == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        cursor += 2;
        while (*cursor && isxdigit((unsigned char)*cursor)) {
            count = count * 16 + (isdigit((unsigned char)*cursor)
                ? (*cursor - '0')
                : (tolower((unsigned char)*cursor) - 'a' + 10));
            cursor++;
        }
    } else {
        while (*cursor && isdigit((unsigned char)*cursor)) {
            count = count * 10 + (*cursor - '0');
            cursor++;
        }
    }

    *out_count = count;
    *pp = cursor;
    return (count > 0 && count <= max_times_count) ? 0 : -1;
}

static void trim_trailing_ws(char *text) {
    int len = (int)strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }
}

static void parser_profile_note_capacity(size_t capacity) {
    if (g_parser_profile_enabled && capacity > g_parser_profile_stats.peak_instruction_capacity) {
        g_parser_profile_stats.peak_instruction_capacity = capacity;
    }
}

static int parse_instruction_profiled(parser_state_t *p, parsed_instruction_t *inst) {
    if (!g_parser_profile_enabled) {
        return parse_instruction(p, inst);
    }

    uint64_t instruction_start_ns = parser_now_ns();
    int parse_result = parse_instruction(p, inst);
    g_parser_profile_stats.parse_instruction_calls++;
    g_parser_profile_stats.parse_instruction_ns += (parser_now_ns() - instruction_start_ns);
    return parse_result;
}

static bool should_keep_instruction(const parsed_instruction_t *inst, token_type_t prev_type) {
    return inst->type != INST_NOP || inst->has_label || prev_type == TOK_IDENTIFIER;
}

static void emit_unknown_instruction_error(parser_state_t *p) {
    char message[256];
    char suggestion[256];
    int distance = 999;
    const char *closest = find_closest_instruction_name(p->current.text, &distance);

    snprintf(message, sizeof(message), "Unknown instruction '%.200s'", p->current.text);
    if (closest && distance <= 2) {
        snprintf(suggestion, sizeof(suggestion),
                 "Did you mean '%s'? Check spelling and operand syntax.", closest);
    } else {
        snprintf(suggestion, sizeof(suggestion),
                 "Check spelling, supported mnemonics, or directive syntax in the assembly reference.");
    }

    parser_error_context_ex(p, "Syntax", message, suggestion);
}

static int parse_comm_lcomm_operands(parser_state_t *p, parsed_instruction_t *inst) {
    if (p->current.type != TOK_IDENTIFIER) {
        parser_error_context(p, ".comm/.lcomm requires a symbol name");
        return -1;
    }
    inst->operands[0].type = OPERAND_LABEL;
    copy_bounded(inst->operands[0].label, sizeof(inst->operands[0].label), p->current.text);
    inst->operand_count = 1;
    advance(p);

    if (p->current.type == TOK_COMMA) {
        advance(p);
    }

    if (p->current.type != TOK_NUMBER) {
        parser_error_context(p, ".comm/.lcomm requires a size");
        return -1;
    }
    inst->operands[1].type = OPERAND_IMM;
    inst->operands[1].immediate = p->current.value;
    inst->operand_count = 2;
    advance(p);

    if (p->current.type == TOK_COMMA) {
        advance(p);
        if (p->current.type != TOK_NUMBER) {
            parser_error_context(p, ".comm/.lcomm alignment must be a number");
            return -1;
        }
        inst->operands[2].type = OPERAND_IMM;
        inst->operands[2].immediate = p->current.value;
        inst->operand_count = 3;
        advance(p);
    }

    return 0;
}

static int parse_operands_until_eol(parser_state_t *p, parsed_instruction_t *inst) {
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

    return 0;
}

/* Parse instruction */
static int parse_instruction(parser_state_t *p, parsed_instruction_t *inst) {
    memset(inst, 0, sizeof(*inst));
    inst->line_number = p->line;
    inst->label_column = 1;
    capture_original_line(p, inst->original_line, sizeof(inst->original_line));


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

    {
        int equ_result = maybe_parse_equ_directive(p, inst);
        if (equ_result < 0) {
            return -1;
        }
        if (equ_result == 1) {
            return 0;
        }
    }

    /* Parse instruction mnemonic */
    if (p->current.type == TOK_IDENTIFIER) {
        inst->type = lookup_instruction_type(p->current.text);

        if (inst->type == INST_UNKNOWN) {
            emit_unknown_instruction_error(p);
            return -1;
        }

        char mnemonic_text[MAX_LINE_LENGTH];
        copy_bounded(mnemonic_text, sizeof(mnemonic_text), p->current.text);
        advance(p);

        {
            int section_result = maybe_apply_section_shorthand(p, inst, mnemonic_text);
            if (section_result < 0) {
                return -1;
            }
            if (section_result == 1) {
                return 0;
            }
        }

        /* Handle .comm and .lcomm directives: .comm symbol, size, alignment */
        if (inst->type == INST_COMM || inst->type == INST_LCOMM) {
            if (parse_comm_lcomm_operands(p, inst) < 0) {
                return -1;
            }
            return 0;
        }
    } else {
        /* Label-only, empty line, or comment */
        inst->type = INST_NOP;
        return 0;
    }

    if (parse_operands_until_eol(p, inst) < 0) {
        return -1;
    }

    /* Disambiguate MOVSD: string instruction (no operands) vs SSE move (XMM operands) */
    if (inst->type == INST_MOVSD && inst->operand_count > 0) {
        /* Has operands - must be SSE MOVSD */
        inst->type = INST_MOVSD_XMM;
    }

    if (validate_sse_operands(p, inst) < 0) {
        return -1;
    }

    return 0;
}

/* Main parse function (internal) */
static parsed_instruction_t *parse_source_internal(const char *source, int *count) {
    uint64_t parse_start_ns = 0;
    parser_state_t p;
    parser_init(&p, source);
    init_tokens(&p);

    /* First pass: count instructions */
    size_t capacity = estimate_instruction_capacity(source);
    int n = 0;
    parsed_instruction_t *insts = malloc(capacity * sizeof(parsed_instruction_t));
    if (!insts) return NULL;

    if (g_parser_profile_enabled) {
        parse_start_ns = parser_now_ns();
        parser_profile_note_capacity(capacity);
    }

    while (p.current.type != TOK_EOF) {
        if ((size_t)n >= capacity) {
            capacity *= 2u;
            parsed_instruction_t *new_insts = realloc(insts, capacity * sizeof(parsed_instruction_t));
            if (!new_insts) {
                free(insts);
                return NULL;
            }
            insts = new_insts;
            if (g_parser_profile_enabled) {
                g_parser_profile_stats.realloc_events++;
                parser_profile_note_capacity(capacity);
            }
        }

        /* Save position before parsing to check for explicit nop vs empty line */
        token_type_t prev_type = p.current.type;
        
        if (parse_instruction_profiled(&p, &insts[n]) != 0) {
            free(insts);
            return NULL;
        }

        if (should_keep_instruction(&insts[n], prev_type)) {
            n++;
        }

        /* After parse_instruction, skip NEWLINE if present */
        if (p.current.type == TOK_NEWLINE) {
            advance(&p);
        }
    }

    if (g_parser_profile_enabled) {
        g_parser_profile_stats.parse_calls++;
        g_parser_profile_stats.parse_ns += (parser_now_ns() - parse_start_ns);
    }

    *count = n;
    return insts;
}

/* Parse and expand times directive: times N instruction -> instruction repeated N times */
static int parse_times_directive(const char *line, int *repeat_count, char *instruction_part) {
    if (!line || !repeat_count || !instruction_part) {
        return -1;
    }

    const char *p = line;

    /* Skip leading whitespace */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Check if line starts with 'times' */
    if (strncasecmp(p, "times", 5) != 0) {
        return -1;  /* Not a times directive */
    }
    p += 5;
    
    /* Skip whitespace after 'times' */
    while (*p && isspace((unsigned char)*p)) p++;
    
    if (parse_times_count(&p, repeat_count) < 0) {
        return -1;
    }
    
    /* Skip whitespace after count */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Copy the instruction part */
    int i = 0;
    while (*p && *p != '\n' && *p != ';' && !(p[0] == '/' && p[1] == '/') &&
           i < MAX_LINE_LENGTH - 1) {
        instruction_part[i++] = *p++;
    }
    instruction_part[i] = '\0';
    
    trim_trailing_ws(instruction_part);
    
    return instruction_part[0] != '\0' ? 0 : -1;  /* Success if we have an instruction */
}

/* Helper to expand times directive even without macro context */
static char *expand_times_only(const char *source) {
    size_t output_size = strlen(source) * 4 + 1;
    char *output = malloc(output_size);
    if (!output) return NULL;
    
    size_t out_pos = 0;
    const char *p = source;
    char line[MAX_LINE_LENGTH];
    
    while (*p) {
        /* Read a line */
        int i = 0;
        const char *line_start = p;
        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;
        
        /* Check for times directive */
        int repeat_count;
        char times_instruction[MAX_LINE_LENGTH];
        if (parse_times_directive(line, &repeat_count, times_instruction) == 0) {
            /* Expand times directive by repeating the instruction */
            for (int r = 0; r < repeat_count; r++) {
                size_t inst_len = strlen(times_instruction);
                if (out_pos + inst_len + 1 < output_size - 1) {
                    memcpy(output + out_pos, times_instruction, inst_len);
                    out_pos += inst_len;
                    output[out_pos++] = '\n';
                }
            }
            continue;
        }
        
        /* Copy line as-is */
        size_t line_len = (size_t)(p - line_start);
        if (out_pos + line_len < output_size - 1) {
            memcpy(output + out_pos, line_start, line_len);
            out_pos += line_len;
        }
    }
    
    output[out_pos] = '\0';
    return output;
}

/* Parse source with optional assembler context for macro preprocessing. */
static parsed_instruction_t *parse_source_common(assembler_context_t *ctx,
                                                 const char *source,
                                                 int *count) {
    /* First, always expand times directive */
    char *times_expanded = expand_times_only(source);
    if (!times_expanded) {
        return parse_source_internal(source, count);
    }

    /* Simple case: no assembler context, parse the times-expanded source */
    if (!ctx) {
        parsed_instruction_t *insts = parse_source_internal(times_expanded, count);
        free(times_expanded);
        return insts;
    }
    
    /* Preprocess macros */
    char *preprocessed = preprocess_macros(ctx, times_expanded);
    free(times_expanded);
    
    if (!preprocessed) {
        parser_diag_ctx(
            ctx,
            "Preprocessor",
            "Macro preprocessing failed",
            "Check .macro/.endm blocks and macro invocation arguments for syntax errors"
        );
        return NULL;
    }
    
    /* Parse the preprocessed source */
    parsed_instruction_t *insts = parse_source_internal(preprocessed, count);
    free(preprocessed);
    
    return insts;
}

/* Main parse function with no assembler context. */
parsed_instruction_t *parse_source(const char *source, int *count) {
    return parse_source_common(NULL, source, count);
}

/* Parse source with assembler context for macro support */
parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count) {
    return parse_source_common(ctx, source, count);
}

void parser_profile_enable(bool enabled) {
    g_parser_profile_enabled = enabled;
}

void parser_profile_reset(void) {
    memset(&g_parser_profile_stats, 0, sizeof(g_parser_profile_stats));
}

void parser_profile_get(parser_profile_stats_t *out_stats) {
    if (!out_stats) {
        return;
    }
    *out_stats = g_parser_profile_stats;
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
    const macro_t *macro = macro_lookup(ctx, name);
    if (macro) {
        return macro->param_count;
    }
    return -1;
}

/* Start defining a new macro */
int macro_define(assembler_context_t *ctx, const char *name) {
    if (!ctx || !name) return -1;
    
    if (ctx->macro_count >= MAX_MACROS) {
        char message[128];
        snprintf(message, sizeof(message), "Too many macros defined (max %d)", MAX_MACROS);
        parser_diag_ctx(
            ctx,
            "Constraint",
            message,
            "Reduce macro count or consolidate repeated macros into parameterized forms"
        );
        return -1;
    }
    
    /* Check for duplicate macro names */
    if (macro_lookup(ctx, name) != NULL) {
        char message[256];
        snprintf(message, sizeof(message), "Macro '%.180s' already defined", name);
        parser_diag_ctx(
            ctx,
            "Constraint",
            message,
            "Rename the macro or remove the duplicate definition"
        );
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
        char message[256];
        snprintf(message, sizeof(message), "Too many parameters for macro '%.160s' (max %d)",
                 macro->name, MAX_MACRO_PARAMS);
        parser_diag_ctx(
            ctx,
            "Constraint",
            message,
            "Reduce parameter count or split macro behavior into multiple macros"
        );
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
        char message[256];
        snprintf(message, sizeof(message), "Macro '%.160s' body too long (max %d lines)",
                 macro->name, MAX_MACRO_BODY_LINES);
        parser_diag_ctx(
            ctx,
            "Constraint",
            message,
            "Split the macro into smaller reusable macros"
        );
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
            
            size_t param_len = (size_t)(param_end - param_start);
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

    const macro_t *macro = macro_lookup(ctx, macro_name);
    if (!macro) {
        char message[256];
        snprintf(message, sizeof(message), "Macro '%.180s' not found", macro_name);
        parser_diag_ctx(
            ctx,
            "Syntax",
            message,
            "Define the macro before use, or fix the macro name spelling"
        );
        return NULL;
    }

    /* Check recursion depth */
    if (ctx->macro_ctx.expansion_depth >= MAX_MACRO_EXPANSION_DEPTH) {
        char message[256];
        snprintf(message, sizeof(message), "Macro expansion depth exceeded for '%.150s' (max %d)",
                 macro_name, MAX_MACRO_EXPANSION_DEPTH);
        parser_diag_ctx(
            ctx,
            "Constraint",
            message,
            "Check for recursive macro expansion and add a terminating condition"
        );
        return NULL;
    }
    
    /* Increment expansion depth */
    ctx->macro_ctx.expansion_depth++;
    ctx->macro_ctx.local_label_counter++;
    int local_counter = ctx->macro_ctx.local_label_counter;
    copy_bounded(ctx->macro_ctx.macro_name, sizeof(ctx->macro_ctx.macro_name), macro_name);
    
    /* Allocate buffer for expanded macro */
    size_t buf_size = (size_t)macro->body_line_count * (size_t)MAX_LINE_LENGTH * 2u;
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
    if (!line || !macro_name || !params || !param_count) return -1;
    
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
        parser_diag_ctx(
            NULL,
            "Syntax",
            ".macro requires a name",
            "Use: .macro <name> [param1, param2, ...]"
        );
        return -1;
    }
    
    int i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '.') && i < MAX_LABEL_LENGTH - 1) {
        macro_name[i++] = *p++;
    }
    macro_name[i] = '\0';
    
    if (i == 0) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "Invalid macro name",
            "Use letters, digits, underscore, or dot in macro names"
        );
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
    if (!ctx || !line || !macro_name || !args || !arg_count) return false;
    
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
        copy_bounded(out_dir, MAX_FILEPATH_LENGTH, ".");
        return;
    }

    slash = strrchr(path, '/');
    if (!slash) {
        copy_bounded(out_dir, MAX_FILEPATH_LENGTH, ".");
        return;
    }

    if (slash == path) {
        copy_bounded(out_dir, MAX_FILEPATH_LENGTH, "/");
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
        parser_diag_ctx(
            NULL,
            "Syntax",
            ".include directive requires a quoted filename",
            "Use: .include \"path/to/file.inc\""
        );
        return -1;
    }

    quote = *p++;
    while (*p && *p != quote && i < filename_size - 1) {
        filename[i++] = *p++;
    }
    filename[i] = '\0';

    if (*p != quote) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "Unterminated filename in .include directive",
            "Close the filename with a matching quote"
        );
        return -1;
    }

    if (i == 0) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "Empty filename in .include directive",
            "Provide a non-empty include path between quotes"
        );
        return -1;
    }

    return 1;
}

#define PP_MAX_DEFINES 128
#define PP_MAX_COND_DEPTH 64

typedef struct {
    char name[MAX_LABEL_LENGTH];
    char value[MAX_LINE_LENGTH];
    bool defined;
} pp_define_t;

typedef struct {
    bool parent_active;
    bool branch_taken;
} pp_cond_state_t;

static const char *pp_skip_ws(const char *p) {
    if (!p) {
        return NULL;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static bool pp_is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool pp_is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static pp_define_t *pp_lookup_define(pp_define_t *defines, int define_count, const char *name) {
    for (int i = 0; i < define_count; i++) {
        if (defines[i].defined && strcmp(defines[i].name, name) == 0) {
            return &defines[i];
        }
    }
    return NULL;
}

static int pp_define_set(pp_define_t *defines, int *define_count,
                         const char *name, const char *value) {
    pp_define_t *existing;

    if (!name || !name[0]) {
        return -1;
    }

    existing = pp_lookup_define(defines, *define_count, name);
    if (existing) {
        copy_bounded(existing->value, sizeof(existing->value), value ? value : "");
        return 0;
    }

    if (*define_count >= PP_MAX_DEFINES) {
        char message[128];
        snprintf(message, sizeof(message), "Too many %%define symbols (max %d)", PP_MAX_DEFINES);
        parser_diag_ctx(NULL,
                        "Constraint",
                        message,
                        "Reduce the number of symbols or reuse existing definitions");
        return -1;
    }

    copy_bounded(defines[*define_count].name, sizeof(defines[*define_count].name), name);
    copy_bounded(defines[*define_count].value, sizeof(defines[*define_count].value), value ? value : "");
    defines[*define_count].defined = true;
    (*define_count)++;
    return 0;
}

static int pp_parse_define_directive(const char *line, char *name, char *value) {
    const char *p = pp_skip_ws(line);
    int i = 0;

    if (!p || !name || !value) {
        return -1;
    }

    if (strncasecmp(p, "%define", 7) != 0) {
        return 0;
    }

    if (strlen(p) > 7 && !isspace((unsigned char)p[7])) {
        return 0;
    }

    p = pp_skip_ws(p + 7);
    if (!pp_is_ident_start(*p)) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "%define requires a symbol name",
            "Use: %define SYMBOL value"
        );
        return -1;
    }

    while (*p && pp_is_ident_char(*p) && i < MAX_LABEL_LENGTH - 1) {
        name[i++] = *p++;
    }
    name[i] = '\0';

    p = pp_skip_ws(p);
    copy_bounded(value, MAX_LINE_LENGTH, p);

    /* Trim trailing whitespace from replacement value. */
    i = (int)strlen(value);
    while (i > 0 && isspace((unsigned char)value[i - 1])) {
        value[--i] = '\0';
    }

    return 1;
}

static int pp_parse_symbol_directive(const char *line, const char *directive,
                                     char *name_out) {
    const char *p = pp_skip_ws(line);
    int i = 0;

    if (!p || !directive || !name_out) {
        return -1;
    }

    size_t directive_len = strlen(directive);

    if (strncasecmp(p, directive, directive_len) != 0) {
        return 0;
    }

    if (strlen(p) > directive_len && !isspace((unsigned char)p[directive_len])) {
        return 0;
    }

    p = pp_skip_ws(p + directive_len);
    if (!pp_is_ident_start(*p)) {
        char message[128];
        snprintf(message, sizeof(message), "%s requires a symbol name", directive);
        parser_diag_ctx(NULL,
                        "Syntax",
                        message,
                        "Provide a valid symbol name after the directive");
        return -1;
    }

    while (*p && pp_is_ident_char(*p) && i < MAX_LABEL_LENGTH - 1) {
        name_out[i++] = *p++;
    }
    name_out[i] = '\0';
    return 1;
}

static int pp_eval_number_or_symbol(const char *expr,
                                    pp_define_t *defines, int define_count,
                                    int *result) {
    const char *p = pp_skip_ws(expr);
    int negate = 0;
    char token[MAX_LINE_LENGTH];
    int i = 0;

    if (!p || !*p || !result) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "%if requires an expression",
            "Use: %if <number|symbol|!symbol>"
        );
        return -1;
    }

    while (*p == '!') {
        negate = !negate;
        p = pp_skip_ws(p + 1);
    }

    if (!*p) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "%if requires an expression",
            "Use: %if <number|symbol|!symbol>"
        );
        return -1;
    }

    while (*p && !isspace((unsigned char)*p) && i < MAX_LINE_LENGTH - 1) {
        token[i++] = *p++;
    }
    token[i] = '\0';

    if (!token[0]) {
        parser_diag_ctx(
            NULL,
            "Syntax",
            "%if requires an expression",
            "Use: %if <number|symbol|!symbol>"
        );
        return -1;
    }

    if (pp_is_ident_start(token[0])) {
        const pp_define_t *def = pp_lookup_define(defines, define_count, token);
        if (!def) {
            *result = negate ? 1 : 0;
            return 0;
        }

        if (def->value[0] == '\0') {
            *result = negate ? 0 : 1;
            return 0;
        }

        errno = 0;
        char *end = NULL;
        long long val = strtoll(def->value, &end, 0);
        if (errno == 0 && end && *pp_skip_ws(end) == '\0') {
            *result = negate ? (val == 0) : (val != 0);
            return 0;
        }

        *result = negate ? 0 : 1;
        return 0;
    }

    errno = 0;
    char *end = NULL;
    long long val = strtoll(token, &end, 0);
    if (errno != 0 || !end || *pp_skip_ws(end) != '\0') {
        char message[192];
        snprintf(message, sizeof(message), "Unsupported %%if expression '%.150s'", token);
        parser_diag_ctx(NULL,
                        "Syntax",
                        message,
                        "Use a numeric literal or a defined symbol in %if expressions");
        return -1;
    }

    *result = negate ? (val == 0) : (val != 0);
    return 0;
}

static int pp_parse_expr_directive(const char *line, const char *directive,
                                   char *expr_out) {
    const char *p = pp_skip_ws(line);
    if (!p || !directive || !expr_out) {
        return -1;
    }

    size_t n = strlen(directive);

    if (strncasecmp(p, directive, n) != 0) {
        return 0;
    }

    if (strlen(p) > n && !isspace((unsigned char)p[n])) {
        return 0;
    }

    p = pp_skip_ws(p + n);
    copy_bounded(expr_out, MAX_LINE_LENGTH, p);
    return 1;
}

static int pp_substitute_defines(const char *line, pp_define_t *defines, int define_count,
                                 char *out, size_t out_size) {
    const char *p = line;
    size_t pos = 0;
    bool in_single = false;
    bool in_double = false;

    if (out_size == 0) return -1;

    while (*p && pos + 1 < out_size) {
        if (!in_single && *p == '"') {
            in_double = !in_double;
            out[pos++] = *p++;
            continue;
        }
        if (!in_double && *p == '\'') {
            in_single = !in_single;
            out[pos++] = *p++;
            continue;
        }

        if (!in_single && !in_double && pp_is_ident_start(*p)) {
            char name[MAX_LABEL_LENGTH];
            int i = 0;
            const char *start = p;
            const pp_define_t *def;

            while (*p && pp_is_ident_char(*p) && i < MAX_LABEL_LENGTH - 1) {
                name[i++] = *p++;
            }
            name[i] = '\0';
            def = pp_lookup_define(defines, define_count, name);
            if (def) {
                size_t val_len = strlen(def->value);
                if (pos + val_len + 1 >= out_size) {
                    return -1;
                }
                memcpy(out + pos, def->value, val_len);
                pos += val_len;
                continue;
            }

            /* No define match, copy original token. */
            size_t token_len = (size_t)(p - start);
            if (pos + token_len + 1 >= out_size) {
                return -1;
            }
            memcpy(out + pos, start, token_len);
            pos += token_len;
            continue;
        }

        out[pos++] = *p++;
    }

    if (*p != '\0') {
        return -1;
    }

    out[pos] = '\0';
    return 0;
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
        char message[128];
        snprintf(message, sizeof(message), "Include depth exceeded (max %d)", MAX_INCLUDE_DEPTH);
        parser_diag_ctx(ctx,
                        "Constraint",
                        message,
                        "Reduce nested include chains or flatten include hierarchy");
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
                copy_bounded(resolved_path, sizeof(resolved_path), include_name);
            } else {
                if (!base_dir || !base_dir[0] || strcmp(base_dir, ".") == 0) {
                    if (snprintf(resolved_path, sizeof(resolved_path), "%s", include_name) >= (int)sizeof(resolved_path)) {
                        char message[192];
                        snprintf(message, sizeof(message), "Include path too long: %.160s", include_name);
                        parser_diag_ctx(ctx,
                                        "Constraint",
                                        message,
                                        "Shorten the include path or move the file closer to the source directory");
                        free(output);
                        return NULL;
                    }
                } else {
                    if (snprintf(resolved_path, sizeof(resolved_path), "%s/%s", base_dir, include_name) >= (int)sizeof(resolved_path)) {
                        char message[320];
                        snprintf(message, sizeof(message), "Include path too long: %.120s/%.120s", base_dir, include_name);
                        parser_diag_ctx(ctx,
                                        "Constraint",
                                        message,
                                        "Shorten the include path or use a simpler include directory structure");
                        free(output);
                        return NULL;
                    }
                }
            }

            if (ctx && is_circular_include(ctx, resolved_path)) {
                char message[256];
                snprintf(message, sizeof(message), "Circular include detected: '%.180s'", resolved_path);
                parser_diag_ctx(ctx,
                                "Constraint",
                                message,
                                "Remove cyclical include relationships between files");
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
        if (had_newline && append_text(&output, &output_capacity, &output_len, "\n", 1) < 0) {
            free(output);
            return NULL;
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
    pp_define_t defines[PP_MAX_DEFINES];
    pp_cond_state_t cond_stack[PP_MAX_COND_DEPTH];
    int define_count = 0;
    int cond_depth = 0;
    bool current_active = true;
    char line[MAX_LINE_LENGTH];
    const char *p;

    dirname_from_path(ctx->current_filename, base_dir);
    expanded_source = expand_includes_recursive(ctx, source, base_dir, 0);
    if (!expanded_source) return NULL;
    source_text = expanded_source;

    ctx->macro_count = 0;
    ctx->in_macro_definition = false;
    ctx->current_macro = NULL;
    ctx->macro_ctx.expansion_depth = 0;
    ctx->macro_ctx.local_label_counter = 0;
    ctx->macro_ctx.macro_name[0] = '\0';
    memset(defines, 0, sizeof(defines));

    p = source_text;
    while (*p) {
        int i = 0;
        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) line[i++] = *p++;
        line[i] = '\0';
        if (*p == '\n') p++;

        {
            char expr[MAX_LINE_LENGTH];
            char symbol[MAX_LABEL_LENGTH];
            int parsed = pp_parse_expr_directive(line, "%if", expr);
            if (parsed < 0) { free(expanded_source); return NULL; }
            if (parsed == 1) {
                int cond_value = 0;
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(expanded_source);
                    return NULL;
                }
                if (current_active && pp_eval_number_or_symbol(expr, defines, define_count, &cond_value) < 0) {
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && (cond_value != 0);
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            parsed = pp_parse_symbol_directive(line, "%ifdef", symbol);
            if (parsed < 0) { free(expanded_source); return NULL; }
            if (parsed == 1) {
                bool defined = pp_lookup_define(defines, define_count, symbol) != NULL || is_macro_name(ctx, symbol);
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && defined;
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            parsed = pp_parse_symbol_directive(line, "%ifndef", symbol);
            if (parsed < 0) { free(expanded_source); return NULL; }
            if (parsed == 1) {
                bool defined = pp_lookup_define(defines, define_count, symbol) != NULL || is_macro_name(ctx, symbol);
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && !defined;
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            if (strncasecmp(pp_skip_ws(line), "%else", 5) == 0) {
                if (cond_depth <= 0) {
                    parser_diag_ctx(
                        ctx,
                        "Syntax",
                        "%else without matching %if",
                        "Add a matching %if/%ifdef/%ifndef before %else"
                    );
                    free(expanded_source);
                    return NULL;
                }
                current_active = cond_stack[cond_depth - 1].parent_active && !cond_stack[cond_depth - 1].branch_taken;
                cond_stack[cond_depth - 1].branch_taken = true;
                continue;
            }

            if (strncasecmp(pp_skip_ws(line), "%endif", 6) == 0) {
                if (cond_depth <= 0) {
                    parser_diag_ctx(
                        ctx,
                        "Syntax",
                        "%endif without matching %if",
                        "Add a matching %if/%ifdef/%ifndef before %endif"
                    );
                    free(expanded_source);
                    return NULL;
                }
                cond_depth--;
                current_active = cond_stack[cond_depth].parent_active;
                continue;
            }

            if (!current_active) continue;

            {
                char def_name[MAX_LABEL_LENGTH];
                char def_value[MAX_LINE_LENGTH];
                int def_parsed = pp_parse_define_directive(line, def_name, def_value);
                if (def_parsed < 0) { free(expanded_source); return NULL; }
                if (def_parsed == 1) {
                    if (pp_define_set(defines, &define_count, def_name, def_value) < 0) {
                        free(expanded_source);
                        return NULL;
                    }
                    continue;
                }
            }
        }

        {
            char macro_name[MAX_LABEL_LENGTH];
            char params[MAX_MACRO_PARAMS][MAX_LABEL_LENGTH];
            int param_count = 0;

            if (parse_macro_directive(line, macro_name, params, &param_count) == 0) {
                if (macro_define(ctx, macro_name) < 0) {
                    free(expanded_source);
                    return NULL;
                }
                for (int j = 0; j < param_count; j++) macro_add_param(ctx, params[j]);

                while (*p) {
                    i = 0;
                    while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) line[i++] = *p++;
                    line[i] = '\0';
                    if (*p == '\n') {
                        line[i++] = '\n';
                        line[i] = '\0';
                        p++;
                    }
                    if (is_endm_directive(line)) {
                        macro_end_definition(ctx);
                        break;
                    }
                    if (i > 0 && line[i - 1] == '\n') line[i - 1] = '\0';
                    macro_add_body_line(ctx, line);
                }
            }
        }
    }

    if (cond_depth != 0) {
        parser_diag_ctx(
            ctx,
            "Syntax",
            "Unterminated preprocessor conditional block",
            "Close all active preprocessor conditionals with %endif"
        );
        free(expanded_source);
        return NULL;
    }

    size_t output_capacity = strlen(source_text) * 4 + 1;
    char *output = malloc(output_capacity);
    if (!output) {
        free(expanded_source);
        return NULL;
    }
    output[0] = '\0';
    size_t output_len = 0;

    define_count = 0;
    memset(defines, 0, sizeof(defines));
    cond_depth = 0;
    current_active = true;
    p = source_text;

    while (*p) {
        int i = 0;
        while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) line[i++] = *p++;
        line[i] = '\0';
        if (*p == '\n') p++;

        {
            char expr[MAX_LINE_LENGTH];
            char symbol[MAX_LABEL_LENGTH];
            int parsed = pp_parse_expr_directive(line, "%if", expr);
            if (parsed < 0) { free(output); free(expanded_source); return NULL; }
            if (parsed == 1) {
                int cond_value = 0;
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                if (current_active && pp_eval_number_or_symbol(expr, defines, define_count, &cond_value) < 0) {
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && (cond_value != 0);
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            parsed = pp_parse_symbol_directive(line, "%ifdef", symbol);
            if (parsed < 0) { free(output); free(expanded_source); return NULL; }
            if (parsed == 1) {
                bool defined = pp_lookup_define(defines, define_count, symbol) != NULL || is_macro_name(ctx, symbol);
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && defined;
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            parsed = pp_parse_symbol_directive(line, "%ifndef", symbol);
            if (parsed < 0) { free(output); free(expanded_source); return NULL; }
            if (parsed == 1) {
                bool defined = pp_lookup_define(defines, define_count, symbol) != NULL || is_macro_name(ctx, symbol);
                if (cond_depth >= PP_MAX_COND_DEPTH) {
                    char message[160];
                    snprintf(message, sizeof(message), "Preprocessor conditional nesting too deep (max %d)", PP_MAX_COND_DEPTH);
                    parser_diag_ctx(ctx,
                                    "Constraint",
                                    message,
                                    "Reduce nested %if/%ifdef/%ifndef blocks");
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                cond_stack[cond_depth].parent_active = current_active;
                cond_stack[cond_depth].branch_taken = current_active && !defined;
                current_active = cond_stack[cond_depth].branch_taken;
                cond_depth++;
                continue;
            }

            if (strncasecmp(pp_skip_ws(line), "%else", 5) == 0) {
                if (cond_depth <= 0) {
                    parser_diag_ctx(
                        ctx,
                        "Syntax",
                        "%else without matching %if",
                        "Add a matching %if/%ifdef/%ifndef before %else"
                    );
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                current_active = cond_stack[cond_depth - 1].parent_active && !cond_stack[cond_depth - 1].branch_taken;
                cond_stack[cond_depth - 1].branch_taken = true;
                continue;
            }

            if (strncasecmp(pp_skip_ws(line), "%endif", 6) == 0) {
                if (cond_depth <= 0) {
                    parser_diag_ctx(
                        ctx,
                        "Syntax",
                        "%endif without matching %if",
                        "Add a matching %if/%ifdef/%ifndef before %endif"
                    );
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                cond_depth--;
                current_active = cond_stack[cond_depth].parent_active;
                continue;
            }

            {
                char msg[MAX_LINE_LENGTH];
                int err_parse = pp_parse_expr_directive(line, "%error", msg);
                if (err_parse < 0) { free(output); free(expanded_source); return NULL; }
                if (err_parse == 1 && current_active) {
                    parser_diag_ctx(ctx,
                                    "Preprocessor",
                                    pp_skip_ws(msg),
                                    "Resolve the %error condition or adjust preprocessor guards");
                    free(output);
                    free(expanded_source);
                    return NULL;
                }

                int warn_parse = pp_parse_expr_directive(line, "%warning", msg);
                if (warn_parse < 0) { free(output); free(expanded_source); return NULL; }
                if (warn_parse == 1 && current_active) {
                    fprintf(stderr, "Warning: %s\n", pp_skip_ws(msg));
                    continue;
                }
            }

            {
                char def_name[MAX_LABEL_LENGTH];
                char def_value[MAX_LINE_LENGTH];
                int def_parsed = pp_parse_define_directive(line, def_name, def_value);
                if (def_parsed < 0) { free(output); free(expanded_source); return NULL; }
                if (def_parsed == 1 && current_active) {
                    if (pp_define_set(defines, &define_count, def_name, def_value) < 0) {
                        free(output);
                        free(expanded_source);
                        return NULL;
                    }
                    continue;
                }
            }
        }

        if (!current_active) continue;

        {
            char test_name[MAX_LABEL_LENGTH];
            char test_params[MAX_MACRO_PARAMS][MAX_LABEL_LENGTH];
            int test_count = 0;

            if (parse_macro_directive(line, test_name, test_params, &test_count) == 0) {
                while (*p) {
                    i = 0;
                    while (*p && *p != '\n' && i < MAX_LINE_LENGTH - 1) line[i++] = *p++;
                    line[i] = '\0';
                    if (*p == '\n') p++;
                    if (is_endm_directive(line)) break;
                }
                continue;
            }
        }

        {
            char substituted_line[MAX_LINE_LENGTH * 4];
            if (pp_substitute_defines(line, defines, define_count,
                                      substituted_line, sizeof(substituted_line)) < 0) {
                parser_diag_ctx(
                    ctx,
                    "Constraint",
                    "Expanded line exceeds preprocessor buffer capacity",
                    "Shorten macro expansions or split generated output across lines"
                );
                free(output);
                free(expanded_source);
                return NULL;
            }

            int repeat_count;
            char times_instruction[MAX_LINE_LENGTH];
            if (parse_times_directive(substituted_line, &repeat_count, times_instruction) == 0) {
                for (int r = 0; r < repeat_count; r++) {
                    if (append_text(&output, &output_capacity, &output_len,
                                    times_instruction, strlen(times_instruction)) < 0 ||
                        append_text(&output, &output_capacity, &output_len, "\n", 1) < 0) {
                        parser_diag_ctx(
                            ctx,
                            "Constraint",
                            "Preprocessor output exceeded internal buffer capacity",
                            "Reduce macro/times expansion size or split output into smaller blocks"
                        );
                        free(output);
                        free(expanded_source);
                        return NULL;
                    }
                }
                continue;
            }

            char macro_name[MAX_LABEL_LENGTH];
            char args[MAX_MACRO_PARAMS][MAX_LINE_LENGTH];
            int arg_count = 0;

            if (parse_macro_invocation(ctx, substituted_line, macro_name, args, &arg_count)) {
                const char *arg_ptrs[MAX_MACRO_PARAMS];
                char expanded_substituted[MAX_LINE_LENGTH * 8];
                for (int j = 0; j < arg_count; j++) arg_ptrs[j] = args[j];

                int expanded_lines = 0;
                char *expanded = expand_macro(ctx, macro_name, arg_ptrs, arg_count, &expanded_lines);
                (void)expanded_lines;
                if (!expanded) {
                    free(output);
                    free(expanded_source);
                    return NULL;
                }

                if (pp_substitute_defines(expanded, defines, define_count,
                                          expanded_substituted, sizeof(expanded_substituted)) < 0) {
                    parser_diag_ctx(
                        ctx,
                        "Constraint",
                        "Expanded macro output exceeded internal buffer capacity",
                        "Reduce expansion size or break the macro into smaller macros"
                    );
                    free(expanded);
                    free(output);
                    free(expanded_source);
                    return NULL;
                }

                if (append_text(&output, &output_capacity, &output_len,
                                expanded_substituted, strlen(expanded_substituted)) < 0) {
                    parser_diag_ctx(
                        ctx,
                        "Constraint",
                        "Preprocessor output exceeded internal buffer capacity",
                        "Reduce macro expansion output or split generated lines"
                    );
                    free(expanded);
                    free(output);
                    free(expanded_source);
                    return NULL;
                }
                free(expanded);
                continue;
            }

            if (append_text(&output, &output_capacity, &output_len,
                            substituted_line, strlen(substituted_line)) < 0 ||
                append_text(&output, &output_capacity, &output_len, "\n", 1) < 0) {
                parser_diag_ctx(
                    ctx,
                    "Constraint",
                    "Preprocessor output exceeded internal buffer capacity",
                    "Reduce macro/times output size or split into smaller source units"
                );
                free(output);
                free(expanded_source);
                return NULL;
            }
        }
    }

    if (cond_depth != 0) {
        parser_diag_ctx(
            ctx,
            "Syntax",
            "Unterminated preprocessor conditional block",
            "Close all active preprocessor conditionals with %endif"
        );
        free(output);
        free(expanded_source);
        return NULL;
    }

    free(expanded_source);
    return output;
}

/* Read entire file contents into a malloc'd buffer */
char *read_file_contents(const char *filename) {
    if (!filename) return NULL;

    FILE *f = fopen(filename, "r");
    if (!f) {
        char message[320];
        snprintf(message, sizeof(message), "Cannot open include file '%.160s': %.80s", filename, strerror(errno));
        parser_diag_ctx(NULL,
                        "IO",
                        message,
                        "Verify the include path and file permissions");
        return NULL;
    }

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

    size_t size_u = (size_t)size;

    char *content = malloc(size_u + 2u);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1u, size_u, f);
    fclose(f);

    if (read != size_u) {
        free(content);
        return NULL;
    }

    content[size_u] = '\n';
    content[size_u + 1u] = '\0';
    return content;
}

/* Check if a file is already in the include chain (circular include detection) */
bool is_circular_include(const assembler_context_t *ctx, const char *filename) {
    if (!ctx || !filename) return false;
    
    for (int i = 0; i < ctx->include_ctx.tracker.count; i++) {
        if (strcmp(ctx->include_ctx.tracker.filenames[i], filename) == 0) {
            return true;
        }
    }
    return false;
}

/* Push a new file onto the include stack */
int include_push(assembler_context_t *ctx, const char *filename, char *content) {
    if (!ctx || !filename || !content) return -1;
    
    /* Check depth limit */
    if (ctx->include_ctx.depth >= MAX_INCLUDE_DEPTH) {
        char message[320];
        snprintf(message, sizeof(message), "Include depth exceeded (max %d) when including '%.150s'",
                 MAX_INCLUDE_DEPTH, filename);
        parser_diag_ctx(ctx,
                        "Constraint",
                        message,
                        "Reduce nested include chains or flatten include hierarchy");
        return -1;
    }
    
    /* Check for circular includes */
    if (is_circular_include(ctx, filename)) {
        char message[320];
        snprintf(message, sizeof(message), "Circular include detected: '%.160s' is already being processed", filename);
        parser_diag_ctx(ctx,
                        "Constraint",
                        message,
                        "Remove cyclical include relationships between files");
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
    entry->source = content;
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
        const file_entry_t *prev = &ctx->include_ctx.stack[ctx->include_ctx.depth - 1];
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
void get_source_location(const assembler_context_t *ctx, char *filename, int *line_number) {
    if (!ctx) {
        if (filename) filename[0] = '\0';
        if (line_number) *line_number = 0;
        return;
    }
    
    if (filename) {
        copy_bounded(filename, MAX_FILEPATH_LENGTH, ctx->current_filename);
    }
    
    if (line_number) {
        *line_number = ctx->line_number;
    }
}

