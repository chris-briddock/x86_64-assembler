/**
 * x86_64 Assembler - Header File
 * A from-scratch assembler that generates raw binary/machine code
 */

#ifndef X86_64_ASM_H
#define X86_64_ASM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_LINE_LENGTH     1024
#define MAX_LABEL_LENGTH    256
#define MAX_SYMBOLS         4096
#define MAX_LINE_MAP        8192
#define MAX_OUTPUT_SIZE     (1024 * 1024)  /* 1MB max output */
#define MAX_OPERANDS        16  /* Support data directives with multiple values */

/* Macro Support Constants */
#define MAX_MACROS          16   /* Maximum number of macros */
#define MAX_MACRO_PARAMS    8    /* Maximum parameters per macro */
#define MAX_MACRO_BODY_LINES 16  /* Maximum lines in macro body */
#define MAX_MACRO_EXPANSION_DEPTH 10 /* Maximum recursion depth */

/* Include File Support Constants */
#define MAX_INCLUDE_DEPTH   10   /* Maximum nested include depth */
#define MAX_INCLUDE_FILES   64   /* Maximum total files in include chain */
#define MAX_FILEPATH_LENGTH 512  /* Maximum file path length */

/* ELF64 Constants */
#define ELFMAG0             0x7f
#define ELFMAG1             'E'
#define ELFMAG2             'L'
#define ELFMAG3             'F'
#define ELFCLASS64          2
#define ELFDATA2LSB         1
#define EV_CURRENT          1
#define ET_EXEC             2
#define EM_X86_64           62
#define PT_LOAD             1
#define PF_X                1
#define PF_W                2
#define PF_R                4

/* x86_64 Register Encodings */
typedef enum {
    /* 64-bit registers */
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8  = 8, R9  = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15,
    /* 32-bit registers */
    EAX = 0, ECX = 1, EDX = 2, EBX = 3,
    ESP = 4, EBP = 5, ESI = 6, EDI = 7,
    R8D = 8, R9D = 9, R10D = 10, R11D = 11,
    R12D = 12, R13D = 13, R14D = 14, R15D = 15,
    /* 16-bit registers */
    AX = 0, CX = 1, DX = 2, BX = 3,
    SP = 4, BP = 5, SI = 6, DI = 7,
    R8W = 8, R9W = 9, R10W = 10, R11W = 11,
    R12W = 12, R13W = 13, R14W = 14, R15W = 15,
    /* 8-bit registers (low) */
    AL = 0, CL = 1, DL = 2, BL = 3,
    SPL = 4, BPL = 5, SIL = 6, DIL = 7,
    R8B = 8, R9B = 9, R10B = 10, R11B = 11,
    R12B = 12, R13B = 13, R14B = 14, R15B = 15,
    /* 8-bit high registers */
    AH = 4, CH = 5, DH = 6, BH = 7,
    /* XMM registers (128-bit SIMD) */
    XMM0 = 16, XMM1 = 17, XMM2 = 18, XMM3 = 19,
    XMM4 = 20, XMM5 = 21, XMM6 = 22, XMM7 = 23,
    XMM8 = 24, XMM9 = 25, XMM10 = 26, XMM11 = 27,
    XMM12 = 28, XMM13 = 29, XMM14 = 30, XMM15 = 31,
    /* Special registers */
    RIP = 32,      /* Instruction pointer (for RIP-relative addressing) */
    REG_NONE = -1
} reg_t;

typedef enum {
    REG_SIZE_8,
    REG_SIZE_8H,   /* AH, BH, CH, DH */
    REG_SIZE_16,
    REG_SIZE_32,
    REG_SIZE_64,
    REG_SIZE_XMM   /* 128-bit XMM register */
} reg_size_t;

/* Operand Types */
typedef enum {
    OPERAND_NONE,
    OPERAND_REG,           /* Register: rax, ebx, etc. */
    OPERAND_IMM,           /* Immediate: $42, $0x1234 */
    OPERAND_MEM,           /* Memory: [rax], [rbx+4] */
    OPERAND_LABEL,         /* Label reference */
    OPERAND_LABEL_DIFF,    /* Label arithmetic: label1 - label2 */
    OPERAND_STRING         /* String literal */
} operand_type_t;

typedef enum {
    MEM_SEG_DEFAULT = 0,
    MEM_SEG_FS,
    MEM_SEG_GS
} mem_segment_t;

/* Memory operand structure */
typedef struct {
    reg_t base;
    reg_t index;
    int scale;             /* 1, 2, 4, or 8 */
    int32_t displacement;
    bool has_base;
    bool has_index;
    bool has_displacement;
    bool is_absolute;      /* Absolute addressing, e.g. [abs 0x1234] */
    bool is_rip_relative;  /* RIP-relative addressing [label] or [rip + label] */
    mem_segment_t segment_override;
    char label[MAX_LABEL_LENGTH];  /* Label for RIP-relative addressing */
} mem_operand_t;

/* Operand structure */
typedef struct {
    operand_type_t type;
    reg_t reg;
    reg_size_t reg_size;
    int64_t immediate;
    mem_operand_t mem;
    char label[MAX_LABEL_LENGTH];
    char label_rhs[MAX_LABEL_LENGTH];
    char string[MAX_LINE_LENGTH];
} operand_t;

/* Instruction Types */
typedef enum {
    /* Data Movement */
    INST_MOV,
    INST_PUSH,
    INST_POP,
    INST_LEA,
    INST_MOVZX,
    INST_MOVSX,
    INST_CMOVA, INST_CMOVAE, INST_CMOVB, INST_CMOVBE,
    INST_CMOVE, INST_CMOVG, INST_CMOVGE, INST_CMOVL,
    INST_CMOVLE, INST_CMOVNE, INST_CMOVNO, INST_CMOVNP,
    INST_CMOVNS, INST_CMOVO, INST_CMOVP, INST_CMOVS,
    INST_XCHG,
    INST_BSWAP,

    /* Arithmetic */
    INST_ADD,
    INST_ADC,
    INST_SUB,
    INST_SBB,
    INST_MUL,
    INST_IMUL,
    INST_DIV,
    INST_IDIV,
    INST_INC,
    INST_DEC,
    INST_NEG,
    INST_CMP,
    INST_XOR,
    INST_OR,
    INST_AND,
    INST_NOT,
    INST_TEST,

    /* Bit Operations */
    INST_SHL, INST_SHR, INST_SAL, INST_SAR,
    INST_ROL, INST_ROR, INST_RCL, INST_RCR,
    INST_SHLD, INST_SHRD,
    INST_BT, INST_BTS, INST_BTR, INST_BTC,
    INST_BSF, INST_BSR,

    /* Control Flow */
    INST_JMP,
    INST_JA, INST_JAE, INST_JB, INST_JBE,
    INST_JE, INST_JG, INST_JGE, INST_JL,
    INST_JLE, INST_JNE, INST_JNZ, INST_JNO, INST_JNP,
    INST_JNS, INST_JO, INST_JP, INST_JS,
    INST_CALL,
    INST_RET,
    INST_INT,
    INST_SYSCALL,
    INST_SYSRET,

    /* Conditional Set */
    INST_SETA, INST_SETAE, INST_SETB, INST_SETBE,
    INST_SETE, INST_SETG, INST_SETGE, INST_SETL,
    INST_SETLE, INST_SETNE, INST_SETNO, INST_SETNP,
    INST_SETNS, INST_SETO, INST_SETP, INST_SETS,

    /* Stack Operations */
    INST_LEAVE,
    INST_ENTER,

    /* Other */
    INST_NOP,
    INST_HLT,
    INST_CLC, INST_STC, INST_CMC,
    INST_CLD, INST_STD,
    INST_CLI, INST_STI,
    INST_CWD, INST_CDQ, INST_CQO,
    INST_CBW, INST_CWDE, INST_CDQE,

    /* String instructions */
    INST_MOVSB, INST_MOVSW, INST_MOVSD, INST_MOVSQ,

    /* SSE Move Instructions */
    INST_MOVAPS, INST_MOVUPS,
    INST_MOVSS, INST_MOVAPD, INST_MOVUPD, INST_MOVSD_XMM,  /* MOVSD_XMM to disambiguate from string MOVSD */

    /* SSE Arithmetic - Add */
    INST_ADDSS, INST_ADDPS, INST_ADDSD, INST_ADDPD,

    /* SSE Arithmetic - Subtract */
    INST_SUBSS, INST_SUBPS, INST_SUBSD, INST_SUBPD,

    /* SSE Arithmetic - Multiply */
    INST_MULSS, INST_MULPS, INST_MULSD, INST_MULPD,

    /* SSE Arithmetic - Divide */
    INST_DIVSS, INST_DIVPS, INST_DIVSD, INST_DIVPD,

    /* SSE Packed Integer Moves */
    INST_MOVDQA, INST_MOVDQU,

    /* SSE Packed Integer Arithmetic - Add */
    INST_PADDB, INST_PADDW, INST_PADDD, INST_PADDQ,

    /* SSE Packed Integer Arithmetic - Subtract */
    INST_PSUBB, INST_PSUBW, INST_PSUBD, INST_PSUBQ,

    /* SSE Packed Integer Compare */
    INST_PCMPEQB, INST_PCMPEQW, INST_PCMPEQD, INST_PCMPEQQ,
    INST_PCMPGTB, INST_PCMPGTW, INST_PCMPGTD, INST_PCMPGTQ,

    /* SSE Logical Operations */
    INST_PAND, INST_PANDNPD, INST_POR, INST_PXOR,

    /* Pseudo-instructions */
    INST_DB, INST_DW, INST_DD, INST_DQ,
    INST_RESB, INST_RESW, INST_RESD, INST_RESQ,  /* Reserve uninitialized space */
    INST_EQU,                                   /* Constant definition */
    INST_TIMES,                                 /* Repeat directive */
    INST_ALIGN,                                 /* Align directive */
    INST_INCBIN,                                /* Include binary file */
    INST_COMM,                                  /* Common symbol declaration */
    INST_LCOMM,                                 /* Local common symbol declaration */
    INST_GLOBAL, INST_EXTERN, INST_WEAK, INST_HIDDEN, INST_SECTION,

    INST_UNKNOWN = -1
} instruction_type_t;

/* Parsed instruction structure */
typedef struct {
    instruction_type_t type;
    char label[MAX_LABEL_LENGTH];
    bool has_label;
    int label_column;
    operand_t operands[MAX_OPERANDS];
    int operand_count;
    uint64_t address;
    int line_number;
    char original_line[MAX_LINE_LENGTH];
} parsed_instruction_t;

/* Symbol table entry */
typedef struct {
    char name[MAX_LABEL_LENGTH];
    uint64_t address;
    int decl_line;
    int decl_column;
    bool is_resolved;
    bool is_external;
    bool is_global;
    bool is_weak;
    bool is_hidden;
    bool is_function;
    int section;  /* 0 = text, 1 = data */
} symbol_t;

/* Hash table entry for O(1) symbol lookup */
typedef struct hash_entry {
    char name[MAX_LABEL_LENGTH];
    uint64_t address;
    bool is_resolved;
    bool is_global;
    bool is_external;
    int section;
    struct hash_entry *next;
} hash_entry_t;

#define HASH_TABLE_SIZE 1024

/* ============================================================================
 * MACRO SUPPORT
 * ============================================================================ */

/* Macro parameter structure */
typedef struct {
    char name[MAX_LABEL_LENGTH];  /* Parameter name */
} macro_param_t;

/* Macro definition structure */
typedef struct {
    char name[MAX_LABEL_LENGTH];              /* Macro name */
    macro_param_t params[MAX_MACRO_PARAMS];   /* Parameter names */
    int param_count;                          /* Number of parameters */
    char body[MAX_MACRO_BODY_LINES][MAX_LINE_LENGTH]; /* Macro body lines */
    int body_line_count;                      /* Number of body lines */
    bool defined;                             /* Whether macro is defined */
} macro_t;

/* Macro expansion context - tracks nested expansions */
typedef struct {
    char macro_name[MAX_LABEL_LENGTH];        /* Currently expanding macro */
    int expansion_depth;                      /* Current recursion depth */
    int local_label_counter;                  /* For generating unique local labels */
} macro_expansion_context_t;

/* ============================================================================
 * INCLUDE FILE SUPPORT
 * ============================================================================ */

/* File entry for include stack - tracks state of an open file */
typedef struct {
    char filename[MAX_FILEPATH_LENGTH];       /* Current file path */
    char *source;                             /* File content buffer */
    const char *source_pos;                   /* Current position in source */
    int line_number;                          /* Current line number in file */
    FILE *file_handle;                        /* File handle (if reading from stream) */
} file_entry_t;

/* Include file tracking for circular detection */
typedef struct {
    char filenames[MAX_INCLUDE_DEPTH][MAX_FILEPATH_LENGTH];
    int count;                                /* Number of currently open files */
} include_tracker_t;

/* Include context - manages the file stack */
typedef struct {
    file_entry_t stack[MAX_INCLUDE_DEPTH];    /* Stack of open files */
    int depth;                                /* Current stack depth */
    include_tracker_t tracker;                /* Tracks open files for circular detection */
} include_context_t;

/* Assembler context */
typedef struct {
    uint64_t address;
    int line;
} line_map_entry_t;

typedef struct {
    /* Output buffer */
    uint8_t *output;
    size_t output_size;
    size_t output_capacity;

    /* Sections */
    uint8_t *text_section;
    size_t text_size;
    size_t text_capacity;

    uint8_t *data_section;
    size_t data_size;
    size_t data_capacity;

    /* Symbol table - array for iteration, hash table for lookup */
    symbol_t symbols[MAX_SYMBOLS];
    int symbol_count;
    hash_entry_t *symbol_hash[HASH_TABLE_SIZE];  /* Hash table for O(1) lookup */

    /* Fixup list for forward references */
    struct {
        char label[MAX_LABEL_LENGTH];
        uint64_t location;
        int instruction_type;
        int operand_index;
        int size;  /* 1, 2, 4, or 8 bytes */
        bool is_rip_relative;  /* true for RIP-relative addressing */
        int section;  /* Section where fixup applies (0=text, 1=data) */
    } fixups[MAX_SYMBOLS];
    int fixup_count;

    /* Current state */
    uint64_t current_address;
    int current_section;  /* 0 = text, 1 = data */
    int line_number;
    line_map_entry_t line_map[MAX_LINE_MAP];
    int line_map_count;

    /* Source */
    const char *source;
    const char *source_pos;

    /* Macro support */
    macro_t macros[MAX_MACROS];               /* Macro definitions table */
    int macro_count;                          /* Number of defined macros */
    macro_expansion_context_t macro_ctx;      /* Current expansion context */
    bool in_macro_definition;                 /* Currently parsing macro body */
    macro_t *current_macro;                   /* Macro being defined */

    /* Include file support */
    include_context_t include_ctx;              /* File include stack */
    char current_filename[MAX_FILEPATH_LENGTH]; /* Current file for error reporting */

    /* Options */
    bool generate_elf;
    bool emit_debug_map;
    uint64_t base_address;
} assembler_context_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/* Initialization */
assembler_context_t *asm_init(void);
void asm_free(assembler_context_t *ctx);

/* Hash table helpers for O(1) symbol lookup */
hash_entry_t *symbol_hash_lookup(assembler_context_t *ctx, const char *name);
int symbol_hash_insert(assembler_context_t *ctx, const char *name, uint64_t address,
                       bool is_global, bool is_external, int section);
int symbol_hash_update(assembler_context_t *ctx, const char *name, uint64_t address,
                       bool is_global, bool is_external, int section);
bool symbol_exists(assembler_context_t *ctx, const char *name);
hash_entry_t *get_symbol_info(assembler_context_t *ctx, const char *name);

/* Assembly process */
int asm_assemble(assembler_context_t *ctx, const char *source);
int asm_assemble_file(assembler_context_t *ctx, const char *filename);

/* Output generation */
int asm_write_elf64(assembler_context_t *ctx, const char *filename);
int asm_write_binary(assembler_context_t *ctx, const char *filename);
int asm_write_hex(assembler_context_t *ctx, const char *filename);
int asm_write_debug_map(assembler_context_t *ctx, const char *filename);

/* Debug output */
void asm_dump_symbols(assembler_context_t *ctx);
void asm_dump_output(assembler_context_t *ctx);

/* Output buffer operations (used by encoders) */
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

/* Instruction encoding helpers */
int encode_instruction(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_mov(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_arithmetic(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_push_pop(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_jcc(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_call_ret(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_nop(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_hlt(assembler_context_t *ctx, const parsed_instruction_t *inst);

/* Bit manipulation instructions */
int encode_bit_manip(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_shld_shrd(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_bit_scan(assembler_context_t *ctx, const parsed_instruction_t *inst);

/* SSE instruction encoding */
int encode_sse_mov(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_arith(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_arith(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_cmp(assembler_context_t *ctx, const parsed_instruction_t *inst);
int encode_sse_packed_logic(assembler_context_t *ctx, const parsed_instruction_t *inst);

/* ModR/M and SIB encoding helpers */
uint8_t make_modrm(uint8_t mod, uint8_t reg, uint8_t rm);
uint8_t make_sib(uint8_t scale, uint8_t index, uint8_t base);

/* REX prefix */
int needs_rex(const operand_t *op);
uint8_t make_rex(bool w, bool r, bool x, bool b);

/* Register info */
reg_t parse_register(const char *name, reg_size_t *size);
const char *register_name(reg_t reg, reg_size_t size);

/* ============================================================================
 * MACRO FUNCTIONS
 * ============================================================================ */

/* Macro table operations */
macro_t *macro_lookup(assembler_context_t *ctx, const char *name);
int macro_define(assembler_context_t *ctx, const char *name);
void macro_add_param(assembler_context_t *ctx, const char *param_name);
void macro_add_body_line(assembler_context_t *ctx, const char *line);
void macro_end_definition(assembler_context_t *ctx);

/* Macro expansion */
char *macro_expand_line(assembler_context_t *ctx, const char *line, const char **args, int arg_count);
bool is_macro_name(assembler_context_t *ctx, const char *name);
int get_macro_arg_count(assembler_context_t *ctx, const char *name);

/* Preprocessing */
char *preprocess_macros(assembler_context_t *ctx, const char *source);

/* Parser with context for macro support */
parsed_instruction_t *parse_source_with_context(assembler_context_t *ctx, const char *source, int *count);

/* ============================================================================
 * INCLUDE FILE FUNCTIONS
 * ============================================================================ */

/* Include file operations */
int include_file(assembler_context_t *ctx, const char *filename);
int include_push(assembler_context_t *ctx, const char *filename, const char *content);
int include_pop(assembler_context_t *ctx);
bool is_circular_include(assembler_context_t *ctx, const char *filename);
char *read_file_contents(const char *filename);

/* Get next character from source with include support */
int source_getc(assembler_context_t *ctx);

/* Get current line info for error reporting */
void get_source_location(assembler_context_t *ctx, char *filename, int *line_number);

#endif /* X86_64_ASM_H */
