# Not Implemented Assembler Features

This document tracks features that are commonly found in x86_64 assemblers (like NASM, GAS) and their implementation status in this custom assembler.

## Implementation Status Summary

**Last Updated**: 2026-03-24

| Category | Implemented | Remaining |
|----------|-------------|-----------|
| Data Directives | `resb`, `resw`, `resd`, `resq`, `equ`, `times`, `align`, `incbin` | None |
| String/Char Support | String literals, escape sequences, character literals, UTF-8, concatenation | None |
| Section Directives | `.text`, `.data`, `.bss`, `.rodata`, `.comm`, `.lcomm`, named sections, `segment` | None |
| Preprocessor | `.macro`, `.include`, `%define`, `%if/%else/%endif`, `%ifdef/%ifndef`, `%error`, `%warning` | None |

---

## Data Directives

| Directive | Purpose | Status | Notes |
|-----------|---------|--------|-------|
| `resb` | Reserve bytes (uninitialized) | **Implemented** | Emits zeros, tested with unit tests |
| `resw` | Reserve words (uninitialized) | **Implemented** | Emits zeros, tested with unit tests |
| `resd` | Reserve doublewords (uninitialized) | **Implemented** | Emits zeros, tested with unit tests |
| `resq` | Reserve quadwords (uninitialized) | **Implemented** | Emits zeros, tested with unit tests |
| `equ` | Define constant symbol | **Implemented** | Defines absolute constants |
| `times n` | Repeat directive | **Implemented** | Preprocessor expansion, tested |
| `align n` | Align to n-byte boundary | **Implemented** | Power of 2 required, emits NOPs in text, zeros in data |
| `incbin` | Include binary file | **Implemented** | Embeds binary file contents as raw bytes |

---

## String and Character Support

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| String literals (`"text"`) | Define ASCII strings | **Implemented** | Works with `db` directive |
| Character literals (`'A'`) | Define single character | **Implemented** | Works as immediate or with `db` |
| Escape sequences | `\n`, `\t`, `\r`, `\\`, etc. | **Implemented** | Works in strings and characters |
| UTF-8 strings | Unicode support | **Implemented** | Validated, supports 1-4 byte sequences |
| String concatenation | `"Hello" " World"` | **Implemented** | Adjacent strings auto-concatenated |

---

## Section Directives

| Feature | Status | Notes |
|---------|--------|-------|
| `.text` section | **Implemented** | Fully supported |
| `.data` section | **Implemented** | Fully supported |
| `.bss` section | **Implemented** | Now fully working with `resb` etc. |
| `.rodata` section | **Implemented** | Treated as data section |
| `.comm` | **Implemented** | Common symbols with optional alignment |
| `.lcomm` | **Implemented** | Local common symbols with optional alignment |
| `section .name` | **Implemented** | Supports named sections and subsection prefixes |
| `segment` directive | **Implemented** | Alias for `section` |

---

## Preprocessor Features

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| `.macro/.endm` | Multi-line macro | **Implemented** | Working |
| `.include` | Include file | **Implemented** | Working |
| `%define` | Single-line macro | **Implemented** | Symbol replacement in active preprocessor branches |
| `%if/%else/%endif` | Conditional assembly | **Implemented** | Supports numeric/defined-symbol expressions |
| `%ifdef/%ifndef` | Conditional on definition | **Implemented** | Works with `%define` symbols and macro names |
| `%error` | User-defined error | **Implemented** | Aborts preprocessing with user message |
| `%warning` | User-defined warning | **Implemented** | Emits warning and continues preprocessing |

---

## Advanced Memory Operations

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| `rip` relative with displacement | `[rip + offset]` | **Implemented** | Working |
| Complex addressing | `[base + index*scale + disp]` | Partial | Some forms work |
| Absolute addressing | `[abs 0x1234]` | **Implemented** | Parsed and encoded with absolute displacement form |
| FS/GS segment overrides | `fs:[0]` | **Implemented** | Emits FS/GS segment override prefixes |

---

## Symbol and Label Features

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| Local labels (`.label`) | Labels starting with `.` | **Implemented** | Scoped to nearest non-local label |
| Anonymous labels | `@@`, `@F`, `@B` | **Implemented** | Supports forward (`@F`) and backward (`@B`) references |
| Label arithmetic | `label1 - label2` | **Implemented** | Supports `equ` expressions and resolves to absolute constants |
| Weak symbols | `weak` attribute | **Implemented** | Marks symbols as weak in symbol metadata |
| Hidden symbols | `hidden` attribute | **Implemented** | Marks symbols as hidden in symbol metadata |

---

## Floating-Point Support

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| `dd 1.5` | Float constant | Not implemented | Not supported |
| `dq 1.5` | Double constant | Not implemented | Not supported |
| x87 FPU instructions | `fld`, `fstp`, etc. | Not implemented | Not in scope |
| SSE scalar types | Float/Double in XMM | Partial | MOVSS/MOVSD work |

---

## Debug Information

| Feature | Purpose | Status | Notes |
|---------|---------|--------|-------|
| `-g` flag | DWARF debug info | **Implemented** | Basic support working |
| `.line` directive | Line number info | Not implemented | Not supported |
| `.file` directive | File name info | Not implemented | Not supported |
| `.loc` directive | Source location | Not implemented | Not supported |

---

## Usage Examples

### Character Literals (Newly Implemented)

```asm
section .data
    newline:    db '\n'         ; Single character in data
    letter_A:   db 'A'          ; 'A' = 65

section .text
    mov al, 'A'                 ; Load character as immediate
    cmp bl, '\n'                ; Compare with newline
    mov cl, '\t'                ; Load tab character
```

### Reserve Space

```asm
section .bss
buffer:     resb 64      ; Reserve 64 bytes
word_arr:   resw 10      ; Reserve 10 words (20 bytes)
dword_arr:  resd 5       ; Reserve 5 dwords (20 bytes)
qword_arr:  resq 4       ; Reserve 4 qwords (32 bytes)
```

### String Literals

```asm
section .data
hello:      db "Hello, World!", 10, 0
tabbed:     db "Col1\tCol2\tCol3", 10, 0
```

### Constant Definition

```asm
BUFFER_SIZE     equ 256
MAGIC_NUMBER    equ 0x12345678

section .text
    mov rax, BUFFER_SIZE    ; Uses 256
```

### Repeat Directive

```asm
section .text
    times 5 nop             ; Expands to 5 NOP instructions
    times 10 push rax       ; Expands to 10 push instructions
```

---

## Priority Explanation

- **High**: Features that significantly impact usability and are commonly needed
- **Medium**: Features that would improve convenience but have workarounds
- **Low**: Nice-to-have features or edge cases

## Notes

- This assembler prioritizes simplicity and ELF64 output
- Many "unimplemented" features can be worked around with existing directives
- For production use, consider using NASM or GAS for full feature support
- Unit tests added for all newly implemented features
