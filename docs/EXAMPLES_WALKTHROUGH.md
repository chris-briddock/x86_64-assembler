# Example Programs Walkthrough

Detailed explanations of the example programs in the `examples/` directory.

## Table of Contents

1. [exit.asm - Simplest Program](#exitasm---simplest-program)
2. [arith.asm - Arithmetic Operations](#arithasm---arithmetic-operations)
3. [loop.asm - Loops and Conditionals](#loopasm---loops-and-conditionals)
4. [hello.asm - System Calls and Data](#helloasm---system-calls-and-data)
5. [simple.asm - Function Calls](#simpleasm---function-calls)
6. [test_all.asm - Comprehensive Test](#test_allasm---comprehensive-test)
7. [sections_named_segment.asm - Named Sections and Segment Alias](#sections_named_segmentasm---named-sections-and-segment-alias)
8. [preprocessor_features.asm - Preprocessor Directives](#preprocessor_featuresasm---preprocessor-directives)
9. [label_arithmetic_equ.asm - Label Arithmetic in equ](#label_arithmetic_equasm---label-arithmetic-in-equ)
10. [weak_hidden_symbols.asm - Weak and Hidden Symbol Attributes](#weak_hidden_symbolsasm---weak-and-hidden-symbol-attributes)
11. [echo.asm - Read and Echo Input](#echoasm---read-and-echo-input)
12. [echo_with_prompt.asm - Prompted Echo](#echo_with_promptasm---prompted-echo)
13. [macros.asm - Macro Invocation Patterns](#macrosasm---macro-invocation-patterns)
14. [sse_test.asm - Scalar and Move SSE Smoke Test](#sse_testasm---scalar-and-move-sse-smoke-test)
15. [sse_packed_ops.asm - Packed SSE Operations](#sse_packed_opsasm---packed-sse-operations)
16. [test_align.asm - Alignment in Text and Data](#test_alignasm---alignment-in-text-and-data)
17. [test_align_simple.asm - Basic Text Alignment](#test_align_simpleasm---basic-text-alignment)
18. [test_char_literals.asm - Character Literal Forms](#test_char_literalsasm---character-literal-forms)
19. [test_comm_lcomm.asm - Common Symbol Directives](#test_comm_lcommasm---common-symbol-directives)
20. [test_incbin.asm - Binary Include Directive](#test_incbinasm---binary-include-directive)
21. [test_new_features.asm - Mixed Feature Regression](#test_new_featuresasm---mixed-feature-regression)
22. [test_string_concat.asm - String Concatenation](#test_string_concatasm---string-concatenation)
23. [test_times.asm - times Expansion](#test_timesasm---times-expansion)
24. [test_utf8.asm - UTF-8 Data Support](#test_utf8asm---utf-8-data-support)

---

## exit.asm - Simplest Program

The simplest possible program - just exits with a status code.

```asm
section .text
global _start
_start:
    mov rax, $60    ; sys_exit syscall number
    mov rdi, $42    ; exit code (42)
    syscall         ; make the syscall
```

### Exit Walkthrough

1. **`section .text`** - Declares the code section
2. **`global _start`** - Makes `_start` visible to the linker (entry point)
3. **`_start:`** - Label marking the program entry point
4. **`mov rax, $60`** - Loads syscall number for exit (60) into RAX
5. **`mov rdi, $42`** - Loads exit code (42) into RDI (first argument)
6. **`syscall`** - Executes the system call

### Running exit.asm

```bash
./bin/x86_64-asm examples/exit.asm -o exit_program
chmod +x exit_program
./exit_program
echo $?   # Prints: 42
```

---

## arith.asm - Arithmetic Operations

Demonstrates basic arithmetic operations.

```asm
section .text
global _start
_start:
    mov rax, $10        ; rax = 10
    add rax, $5         ; rax = 15 (10 + 5)
    sub rax, $3         ; rax = 12 (15 - 3)
    mov rdi, rax        ; rdi = 12 (exit code)
    mov rax, $60        ; sys_exit
    syscall
```

### Arithmetic Walkthrough

1. **`mov rax, $10`** - Initialize RAX with 10
2. **`add rax, $5`** - Add 5, RAX now contains 15
3. **`sub rax, $3`** - Subtract 3, RAX now contains 12
4. **`mov rdi, rax`** - Copy result to RDI for exit code
5. **`mov rax, $60`** / **`syscall`** - Exit with code 12

### Arithmetic Key Concepts

- **Immediate operands**: Numbers prefixed with `$`
- **Register arithmetic**: Operations modify the destination register
- **Result passing**: Move result to RDI for exit

### Running arith.asm

```bash
./bin/x86_64-asm examples/arith.asm -o arith
chmod +x arith
./arith
echo $?   # Prints: 12
```

---

## loop.asm - Loops and Conditionals

Demonstrates loops using labels and conditional jumps.

```asm
section .text
global _start
_start:
    mov rcx, $5         ; Initialize counter to 5
    mov rax, $0         ; Initialize sum to 0

loop_start:
    add rax, $1         ; Increment sum
    dec rcx             ; Decrement counter
    jne loop_start      ; Jump if not equal to zero

    mov rdi, rax        ; Exit with sum (5)
    mov rax, $60
    syscall
```

### Loop Walkthrough

1. **Initialization**:
   - `rcx = 5` - Loop counter
   - `rax = 0` - Accumulator for sum

2. **Loop Body** (`loop_start:`):
   - `add rax, $1` - Add 1 to sum
   - `dec rcx` - Decrement counter
   - `jne loop_start` - Jump back if rcx != 0

3. **Exit**:
   - After 5 iterations, RAX = 5
   - Exit with sum as exit code

### Loop Key Concepts

- **Labels**: Mark positions in code (`loop_start:`)
- **Conditional jumps**: `jne` (jump if not equal/zero)
- **Counter-controlled loops**: RCX as loop counter

### Loop Execution Trace

| Iteration | RAX | RCX | Action |
| :--- | :--- | :--- | :--- |
| Start | 0 | 5 | Initialize |
| 1 | 1 | 4 | Add 1, dec, jump |
| 2 | 2 | 3 | Add 1, dec, jump |
| 3 | 3 | 2 | Add 1, dec, jump |
| 4 | 4 | 1 | Add 1, dec, jump |
| 5 | 5 | 0 | Add 1, dec, exit |

### Running loop.asm

```bash
./bin/x86_64-asm examples/loop.asm -o loop
chmod +x loop
./loop
echo $?   # Prints: 5
```

---

## hello.asm - System Calls and Data

Writes "Hi" to stdout using sys_write.

```asm
section .data
    msg: db $48, $69, $0A    ; "Hi\n"
    len: equ $3              ; Length = 3

section .text
global _start
_start:
    ; sys_write(1, msg, len)
    mov rax, $1              ; syscall: write
    mov rdi, $1              ; fd: stdout
    lea rsi, [msg]           ; buffer address
    mov rdx, $len            ; length
    syscall

    ; sys_exit(0)
    mov rax, $60             ; syscall: exit
    mov rdi, $0              ; exit code: 0
    syscall
```

### Hello Walkthrough

**Data Section**:

- `msg:` - Label for message data
- `db $48, $69, $0A` - Define bytes: 'H', 'i', '\n'
- `len: equ $3` - Constant defining length

**Text Section**:

1. **`mov rax, $1`** - sys_write syscall number
2. **`mov rdi, $1`** - File descriptor 1 (stdout)
3. **`lea rsi, [msg]`** - Load address of msg into RSI
4. **`mov rdx, $len`** - Message length into RDX
5. **`syscall`** - Execute write
6. Exit with code 0

### Hello Key Concepts

- **Data sections**: `.data` for initialized data
- **LEA instruction**: Load Effective Address (gets pointer, not value)
- **System calls**: Using Linux syscall interface
- **String encoding**: ASCII values as hex bytes

### Running hello.asm

```bash
./bin/x86_64-asm examples/hello.asm -o hello
chmod +x hello
./hello   # Prints: Hi
```

---

## simple.asm - Function Calls

Demonstrates function calls with `call` and `ret`.

```asm
section .text
global _start
_start:
    mov rdi, $7         ; Argument = 7
    call square         ; Call square(7)
    mov rdi, rax        ; Exit with result
    mov rax, $60
    syscall

; Function: square
; Input: RDI = number to square
; Output: RAX = number squared
square:
    mov rax, rdi        ; Copy argument to RAX
    imul rax, rdi       ; RAX = RAX * RDI (square it)
    ret                 ; Return to caller
```

### Simple Walkthrough

**Main Program**:

1. **`mov rdi, $7`** - Pass argument 7 in RDI
2. **`call square`** - Jump to square function, save return address
3. **`mov rdi, rax`** - Move result to RDI for exit
4. Exit with result (49)

**Square Function**:

1. **`mov rax, rdi`** - Copy argument to RAX
2. **`imul rax, rdi`** - Multiply RAX by RDI (square)
3. **`ret`** - Return to caller

### Simple Key Concepts

- **Function calls**: `call` saves return address and jumps
- **Return**: `ret` pops return address and jumps back
- **Argument passing**: RDI for first integer argument (System V AMD64 ABI)
- **Return value**: RAX holds return value

### Simple Execution

```plaintext

square(7):
  RAX = 7
  RAX = 7 * 7 = 49
  Return 49
```

### Running simple.asm

```bash
./bin/x86_64-asm examples/simple.asm -o simple
chmod +x simple
./simple
echo $?   # Prints: 49
```

---

## test_all.asm - Comprehensive Test

Comprehensive test combining multiple features.

```asm
section .data
    value: dq $0        ; 64-bit variable

section .text
global _start
_start:
    ; Test arithmetic
    mov rax, $10
    add rax, $20        ; rax = 30
    sub rax, $5         ; rax = 25

    ; Store to memory
    mov [value], rax    ; value = 25

    ; Load from memory
    mov rbx, [value]    ; rbx = 25

    ; Memory addressing with displacement
    lea rcx, [rbx + 5]  ; rcx = 30

    ; Test conditional
    cmp rcx, $30
    je correct
    mov rdi, $1         ; Exit 1 if wrong
    jmp exit
correct:
    mov rdi, $0         ; Exit 0 if correct
exit:
    mov rax, $60
    syscall
```

### Test All Walkthrough

1. **Arithmetic**: 10 + 20 - 5 = 25
2. **Memory store**: Save 25 to `value`
3. **Memory load**: Load 25 into RBX
4. **Address calculation**: RCX = RBX + 5 = 30
5. **Comparison**: Check if RCX equals 30
6. **Conditional exit**: Exit 0 if correct, 1 if wrong

### Test All Key Concepts

- **Memory operations**: Store and load from data section
- **LEA with displacement**: Computing addresses with offsets
- **Conditional branches**: `je` for equality check
- **Multiple paths**: Different exit codes for success/failure

### Running test_all.asm

```bash
./bin/x86_64-asm examples/test_all.asm -o test_all
chmod +x test_all
./test_all
echo $?   # Prints: 0 (success)
```

---

## sections_named_segment.asm - Named Sections and Segment Alias

Demonstrates named section prefixes and the `segment` alias.

```asm
section .data.runtime
    exit_code: dq $17

segment .text.hot
global _start
_start:
    mov rdi, [exit_code]
    mov rax, $60
    syscall
```

### What This Validates

1. Named variants like `.data.runtime` map to data semantics
2. `segment` works as an alias for `section`
3. Named text variants like `.text.hot` map to text semantics

### Running sections_named_segment.asm

```bash
./bin/x86_64-asm examples/sections_named_segment.asm -o sections_named_segment
chmod +x sections_named_segment
./sections_named_segment
echo $?   # Prints: 17
```

---

## preprocessor_features.asm - Preprocessor Directives

Demonstrates `%define`, conditionals, `%warning`, and safe `%error` usage inside an inactive branch.

```asm
%define USE_FAST_EXIT 1
%define FAST_EXIT_CODE 23

%if USE_FAST_EXIT
%warning Using fast exit path selected by %define
    section .text
    global _start
_start:
    mov rax, $60
    mov rdi, FAST_EXIT_CODE
    syscall
%else
    %error USE_FAST_EXIT disabled unexpectedly
%endif
```

### What This Validates

1. `%define` symbol replacement in active branches
2. `%if/%else/%endif` branch selection
3. `%warning` emits a non-fatal warning
4. `%error` can enforce invariants (inactive branch here, so build succeeds)
5. `%ifdef/%ifndef` checks for defined and missing symbols

### Running preprocessor_features.asm

```bash
./bin/x86_64-asm examples/preprocessor_features.asm -o preprocessor_features
chmod +x preprocessor_features
./preprocessor_features
echo $?   # Prints: 23
```

---

## label_arithmetic_equ.asm - Label Arithmetic in equ

Demonstrates label subtraction in `equ` expressions and using the result as an immediate constant.

```asm
section .text
start:
    nop
end:

DELTA equ end - start

global _start
_start:
    mov rax, $60
    mov rdi, DELTA
    syscall
```

### What This Validates

1. `equ` can evaluate label subtraction expressions
2. The computed value is available as an immediate operand
3. Symbol resolution happens before emission for these constants

### Running label_arithmetic_equ.asm

```bash
./bin/x86_64-asm examples/label_arithmetic_equ.asm -o label_arithmetic_equ
chmod +x label_arithmetic_equ
./label_arithmetic_equ
echo $?   # Prints: 1
```

---

## weak_hidden_symbols.asm - Weak and Hidden Symbol Attributes

Demonstrates symbol visibility/strength attribute directives parsed and applied by the assembler.

```asm
section .text

weak plugin_entry
hidden internal_helper

global _start
_start:
    mov rax, $60
    mov rdi, $0
    syscall

plugin_entry:
    ret

internal_helper:
    ret
```

### What This Validates

1. `weak` / `.weak` directives are accepted with label operands
2. `hidden` / `.hidden` directives are accepted with label operands
3. Symbol attributes are retained in assembler symbol metadata

### Running weak_hidden_symbols.asm

```bash
./bin/x86_64-asm examples/weak_hidden_symbols.asm -o weak_hidden_symbols
chmod +x weak_hidden_symbols
./weak_hidden_symbols
echo $?   # Prints: 0
```

---

## echo.asm - Read and Echo Input

Reads up to 256 bytes from stdin and writes the same bytes back to stdout.

### What This Validates

1. `sys_read` and `sys_write` syscall flow
2. Buffer addressing via `lea`
3. Conditional early exit when no input is read

### Running echo.asm

```bash
./bin/x86_64-asm examples/echo.asm -o echo
chmod +x echo
./echo
```

---

## echo_with_prompt.asm - Prompted Echo

Prints a prompt, reads stdin, then prints a prefix plus the captured input.

### What This Validates

1. Multiple write syscalls in one flow
2. Read-then-echo pipeline behavior
3. Static prompt/output string data in `.data`

### Running echo_with_prompt.asm

```bash
./bin/x86_64-asm examples/echo_with_prompt.asm -o echo_with_prompt
chmod +x echo_with_prompt
./echo_with_prompt
```

---

## macros.asm - Macro Invocation Patterns

Demonstrates `.macro/.endm` definitions with parameters and reusable push/pop helper blocks.

### What This Validates

1. Parameterized macro expansion (for example `set_reg_imm`)
2. Macro invocations that expand to multiple instructions
3. Reusable prologue/epilogue macro-style patterns

### Running macros.asm

```bash
./bin/x86_64-asm examples/macros.asm -o macros
chmod +x macros
./macros
echo $?   # Prints: 0
```

---

## sse_test.asm - Scalar and Move SSE Smoke Test

Exercises a broad SSE move/scalar arithmetic set across memory and register operands.

### What This Validates

1. `movaps/movups/movss` forms
2. Scalar arithmetic (`addss/subss/mulss/divss`, `mulsd`)
3. Usage of higher XMM registers (`xmm10` to `xmm15`)

### Running sse_test.asm

```bash
./bin/x86_64-asm examples/sse_test.asm -o sse_test
chmod +x sse_test
./sse_test
echo $?   # Prints: 0
```

---

## sse_packed_ops.asm - Packed SSE Operations

Demonstrates packed integer SSE arithmetic, compare, and logic instructions with stack-backed memory operands.

### What This Validates

1. Packed moves (`movdqu`)
2. Packed arithmetic (`paddd`, `psubd`)
3. Packed compare/logic (`pcmpeqd`, `pandn`, `pxor`, `por`)

### Running sse_packed_ops.asm

```bash
./bin/x86_64-asm examples/sse_packed_ops.asm -o sse_packed_ops
chmod +x sse_packed_ops
./sse_packed_ops
echo $?   # Prints: 0
```

---

## test_align.asm - Alignment in Text and Data

Demonstrates `align` in both `.data` and `.text` sections.

### What This Validates

1. Data alignment before `dd`/`dq` payloads
2. Text alignment padding before labeled code blocks
3. Mixed alignment granularities (`4`, `8`, `16`)

### Running test_align.asm

```bash
./bin/x86_64-asm examples/test_align.asm -o test_align
chmod +x test_align
./test_align
echo $?   # Prints: 0
```

---

## test_align_simple.asm - Basic Text Alignment

A minimal alignment-focused sample in `.text` using `align 8` and `align 32`.

### What This Validates

1. Deterministic code alignment boundaries
2. NOP padding behavior in text section
3. Label placement after alignment directives

### Running test_align_simple.asm

```bash
./bin/x86_64-asm examples/test_align_simple.asm -o test_align_simple
chmod +x test_align_simple
./test_align_simple
echo $?   # Prints: 1
```

---

## test_char_literals.asm - Character Literal Forms

Shows single-quoted character literals (including escapes) in both data and instructions.

### What This Validates

1. Character literals in `db` declarations
2. Escaped literal forms like `'\n'`, `'\t'`, `'\0'`
3. Character literals as immediate operands

### Running test_char_literals.asm

```bash
./bin/x86_64-asm examples/test_char_literals.asm -o test_char_literals
chmod +x test_char_literals
./test_char_literals
echo $?   # Prints: 65
```

---

## test_comm_lcomm.asm - Common Symbol Directives

Demonstrates `.comm` and `.lcomm` declarations with optional alignments.

### What This Validates

1. Global common symbols (`.comm`)
2. File-local common symbols (`.lcomm`)
3. Addressability of common symbols from code

### Running test_comm_lcomm.asm

```bash
./bin/x86_64-asm examples/test_comm_lcomm.asm -o test_comm_lcomm
chmod +x test_comm_lcomm
./test_comm_lcomm
echo $?   # Prints: 0
```

---

## test_incbin.asm - Binary Include Directive

Shows `incbin` embedding of an external binary payload into `.data`.

### What This Validates

1. Raw byte inclusion from a file path
2. Mixed literal bytes before and after `incbin`
3. Binary payload embedding in final output

### Running test_incbin.asm

```bash
./bin/x86_64-asm examples/test_incbin.asm -o test_incbin
chmod +x test_incbin
./test_incbin
echo $?   # Prints: 0
```

---

## test_new_features.asm - Mixed Feature Regression

Combines several newer directives/features in one file.

### What This Validates

1. `resb/resw/resd/resq` reserve directives
2. `equ` constant usage in instructions
3. `times` repeated instruction expansion

### Running test_new_features.asm

```bash
./bin/x86_64-asm examples/test_new_features.asm -o test_new_features
chmod +x test_new_features
./test_new_features
echo $?   # Prints: 0
```

---

## test_string_concat.asm - String Concatenation

Demonstrates adjacent string literal concatenation, including escape and UTF-8 content.

### What This Validates

1. Adjacent string token merging in `db`
2. Concatenation with escapes and Unicode text
3. Empty-string participation in concatenated literals

### Running test_string_concat.asm

```bash
./bin/x86_64-asm examples/test_string_concat.asm -o test_string_concat
chmod +x test_string_concat
./test_string_concat
echo $?   # Prints: 0
```

---

## test_times.asm - times Expansion

Minimal demonstration of `times` expansion for repeated instructions.

### What This Validates

1. `times` preprocessing expansion behavior
2. Repetition of simple scalar instructions (`nop`)

### Running test_times.asm

```bash
./bin/x86_64-asm examples/test_times.asm -o test_times
chmod +x test_times
./test_times
echo $?   # Prints: 0
```

---

## test_utf8.asm - UTF-8 Data Support

Shows UTF-8 strings in multiple scripts plus emoji inside `db` directives.

### What This Validates

1. UTF-8 pass-through handling in string literals
2. Multi-byte code point storage for mixed-language data
3. UTF-8 strings combined with escape sequences

### Running test_utf8.asm

```bash
./bin/x86_64-asm examples/test_utf8.asm -o test_utf8
chmod +x test_utf8
./test_utf8
echo $?   # Prints: 0
```

---

## Summary of Examples

| Example | Concepts Covered |
| :--- | :--- |
| exit.asm | Basic program structure, syscalls |
| arith.asm | Arithmetic operations, immediates |
| loop.asm | Labels, loops, conditional jumps |
| hello.asm | Data sections, strings, I/O syscalls |
| simple.asm | Functions, call/ret, arguments |
| test_all.asm | Memory operations, comprehensiveness |
| sections_named_segment.asm | Named section prefixes, `segment` alias |
| preprocessor_features.asm | `%define`, conditionals, `%warning`, `%error` |
| label_arithmetic_equ.asm | `equ` label arithmetic (`label1 - label2`) |
| weak_hidden_symbols.asm | `weak`/`hidden` symbol attribute directives |
| echo.asm | stdin read + stdout echo syscall flow |
| echo_with_prompt.asm | Prompted input echo and multi-write flow |
| macros.asm | `.macro` parameter expansion and reusable blocks |
| sse_test.asm | SSE move/scalar arithmetic smoke coverage |
| sse_packed_ops.asm | Packed SSE arithmetic/compare/logic sequence |
| test_align.asm | `align` behavior in data and text sections |
| test_align_simple.asm | Minimal code-section alignment behavior |
| test_char_literals.asm | Character literal and escape forms |
| test_comm_lcomm.asm | `.comm` and `.lcomm` directives |
| test_incbin.asm | `incbin` raw binary inclusion |
| test_new_features.asm | Combined reserve/equ/times regression sample |
| test_string_concat.asm | Adjacent string literal concatenation |
| test_times.asm | `times` repetition behavior |
| test_utf8.asm | UTF-8 literal support in data |

## Next Steps

Try modifying these examples:

1. Change the exit codes
2. Modify the loop counter
3. Add more arithmetic operations
4. Change the string in hello.asm
5. Create your own functions

For more advanced topics, see [ASSEMBLY_REFERENCE.md](ASSEMBLY_REFERENCE.md).
