# Troubleshooting Guide

Common issues and their solutions when using the x86_64 assembler.

## Build Issues

### "gcc: command not found"

**Problem**: GCC is not installed or not in PATH.

**Solution**:

```bash
# Ubuntu/Debian
sudo apt-get install gcc

# Fedora/RHEL
sudo dnf install gcc

# macOS
xcode-select --install
```

### "make: command not found"

**Problem**: Make build tool is not installed.

**Solution**:

```bash
# Ubuntu/Debian
sudo apt-get install make

# Fedora/RHEL
sudo dnf install make
```

### Compilation Errors

**Problem**: Code does not compile with errors about types or functions.

**Solutions**:

1. Ensure you are using C99 or later: `gcc --std=c99`
2. Check that all source files are present in `src/`
3. Clean and rebuild: `make clean && make`

## Assembly Errors

### New Diagnostic Output Format

Parser-side syntax errors now include source context and a caret marker.

**Example**:

```text
Error at line 4: Unexpected token in memory operand
    mov [rbx + rcx, rax
                                    ^
```

Use the caret position as the first place to inspect syntax.

### "Error: mov requires 2 operands at line X"

**Problem**: The MOV instruction was not parsed correctly.

**Common Causes**:

1. **Typo in instruction name**: Check spelling
2. **Missing operand**: Ensure both source and destination are provided
3. **Invalid operand combination**: Some combinations are not supported

**Examples**:

```asm
; Wrong - missing operand
mov rax

; Wrong - memory to memory not allowed
mov [a], [b]

; Correct
mov rax, $42
mov rax, rbx
mov [label], rax
```

### "Error at line X: Unknown instruction"

**Problem**: The assembler does not recognize the instruction.

**Solutions**:

1. Check that the instruction is supported (see README.md)
2. Check spelling - must be exact
3. Both `jne` and `jnz` are supported (synonyms)

**Example**:

```asm
; Both are supported and produce identical code
jne label        ; Jump if not equal
jnz label        ; Jump if not zero (synonym for jne)
```

### "Error: Failed to resolve label 'X'"

**Problem**: A label reference could not be found.

**Common Causes**:

1. **Typo in label name**: Check case sensitivity
2. **Label not defined**: Ensure label exists with colon
3. **Forward reference in data**: Some cases may not work

**Examples**:

```asm
; Correct - label defined before use
my_label:
    mov rax, $1
    jmp my_label

; Wrong - missing colon
my_label          ; Should be: my_label:
    mov rax, $1
```

### "Error at line X: Expected register or number after +/- in memory operand"

**Problem**: Invalid syntax in memory addressing expression.

**Solutions**:

```asm
; Wrong
mov rax, [rbx + ]    ; Missing value after +

; Also wrong
mov rax, [rbx + , rcx] ; Invalid token sequence

; Correct
mov rax, [rbx + 8]   ; Proper displacement
mov rax, [rbx + rcx] ; Base + index
mov rax, [rbx]       ; No displacement
```

### SSE Operand/Form Errors

**Problem**: SSE instruction fails with operand-form diagnostics.

**Common errors**:

1. `SSE move requires at least one XMM operand`
2. `SSE move cannot have two memory operands`
3. `SSE arithmetic destination must be XMM register`
4. `source must be XMM or memory`

**Examples**:

```asm
; Wrong - no XMM operand
movaps [rax], [rbx]

; Wrong - destination must be XMM for arithmetic
addps [rax], xmm1

; Wrong - immediate sources are not supported for SSE arithmetic
addps xmm0, $1

; Correct
movaps xmm0, [rax]
movaps [rax + 16], xmm0
addps xmm0, xmm1
```

**How to debug quickly**:

1. Ensure at least one operand is an XMM register.
2. For arithmetic/packed/compare/logical SSE families, keep destination as XMM register.
3. Use memory or XMM source operands only (no immediates/GPRs).
4. Check [SSE Coverage Matrix](SSE_COVERAGE_MATRIX.md) for currently supported forms.

### High 8-bit Register + REX Conflict

#### Architectural Constraints

**Problem**: You see errors mentioning `AH/BH/CH/DH` with REX-prefixed encodings.

This is an x86_64 architectural constraint, not a temporary assembler bug.

**Example that fails**:

```asm
add ah, r8b
```

**Why it fails**:

- `r8b` requires a REX prefix.
- `ah`/`bh`/`ch`/`dh` are incompatible with REX-prefixed byte encodings.

**Fix**:

- Use `al`/`bl`/`cl`/`dl` or byte forms of extended registers instead of high 8-bit registers.

## Runtime Issues

### "Segmentation fault (core dumped)"

**Problem**: Program crashed during execution.

**Common Causes**:

1. **Writing to read-only memory**:

```asm
section .rodata
msg: db "Hello"

section .text
mov [msg], $0       ; CRASH! .rodata is read-only
```

1. **Uninitialized pointers**:

```asm
section .bss
ptr: dq $0

section .text
mov rax, [ptr]      ; ptr is uninitialized (0)
mov [rax], $1       ; CRASH! Writing to address 0
```

1. **Stack overflow**:

```asm
; Too many nested calls without returns
call recursive_function
```

1. **Wrong system call numbers**:

```asm
mov rax, $999       ; Invalid syscall number
syscall             ; May crash or return error
```

### "Illegal instruction (core dumped)"

**Problem**: CPU encountered an instruction it does not understand.

**Common Causes**:

1. **Corrupted executable**: Rebuild the assembly
2. **Invalid encoding**: May be an assembler bug - report it
3. **Wrong architecture**: Ensure you are on x86_64 Linux

### Program exits with wrong code

**Problem**: Exit code is not what you expected.

**Debugging**:

```bash
# Check exit code
echo $?   # Prints exit code of last command

# Debug with GDB
gdb ./myprogram
(gdb) break _start
(gdb) run
(gdb) info registers
```

**Common Issues**:

```asm
; Forgetting to set RDI (exit code)
mov rax, $60
syscall             ; Exit with whatever was in RDI

; Correct
mov rax, $60
mov rdi, $42        ; Set exit code
syscall
```

## ELF/Output Issues

### "cannot execute binary file: Exec format error"

**Problem**: File is not a valid executable.

**Solutions**:

1. Make sure assembly succeeded (check for errors)
2. Make file executable: `chmod +x output_file`
3. Check file type: `file output_file`

### Output file is 0 bytes

**Problem**: Assembly failed silently or no code generated.

**Solutions**:

1. Check for error messages during assembly
2. Ensure you have a `.text` section with code
3. Verify `global _start` and `_start:` label exist

## Testing Issues

### Why does `make test-unit` show different totals?

`make test-unit` runs two separate suites (`test-encoder`, then `test-parser`).
Each suite prints its own summary, and then a final **Combined Unit Summary** line
is printed with aggregate totals.

Use the final combined block for the overall unit-test count.

### Tests fail with "ASSERTION FAILED"

**Problem**: Test output does not match expected value.

**Debugging**:

1. Look at the test source in `tests/test_*.c`
2. Run the assembler manually with the test input
3. Check the generated binary with `objdump -d` or `xxd`

### "Test framework: Unknown test"

**Problem**: Test function is not registered.

**Solution**: Ensure test is added to the `test_cases[]` array in the test file.

## Performance Issues

### Assembly takes a long time

**Problem**: Large files assemble slowly.

**Current Limitations**:

- Multi-pass assembly and fixup/debug processing can be slower on very large sources
- No optimization passes (assembler prioritizes correctness and clarity)

**Solutions**:

1. Split large files into smaller ones
2. Remove unnecessary labels
3. Use `make -j` for parallel builds of multiple files

## Common Mistakes

### Forgetting $ for immediates

```asm
; Wrong - treats 42 as memory address
mov rax, 42

; Correct
mov rax, $42
```

### Confusing [] with memory access

```asm
; Wrong - loads address, not value
mov rax, label

; Correct - loads value from label address
mov rax, [label]

; Correct - gets address of label
lea rax, [label]
```

### Stack imbalance

```asm
; Wrong - push without pop
push rax
; ... code ...
ret                 ; Stack unbalanced!

; Correct
push rax
; ... code ...
pop rax             ; Restore stack
ret
```

### Wrong register size

```asm
; Wrong - 32-bit mov zero-extends to 64-bit
mov eax, $-1        ; RAX becomes 0x00000000FFFFFFFF
mov rdi, rax        ; Exit code 255, not -1

; Correct - use 64-bit immediate for negative values
mov rax, $-1        ; RAX becomes 0xFFFFFFFFFFFFFFFF
mov rdi, rax        ; Exit code -1 (or 255 if unsigned)
```

## Getting Help

If you encounter an issue not covered here:

1. **Check the examples**: Look at `examples/*.asm` for working code
2. **Run tests**: `make test-unit test-integration` to verify the assembler
3. **Enable debugging**: Build with `CFLAGS="-g -DDEBUG" make`
4. **Check issues**: Look for similar issues in the project tracker
5. **Create a minimal example**: Reduce your code to the smallest failing case

## Debug Techniques

### Enable Verbose Output

Modify the assembler to add debug prints:

```c
// In x86_64_asm.c, add before encoding:
printf("Encoding instruction type %d at line %d\n",
       inst->type, inst->line_number);
```

### Inspect Generated Code

```bash
# View hex dump
xxd output_file

# Disassemble (if symbols available)
objdump -d output_file

# Check ELF headers
readelf -h output_file
readelf -S output_file  # Section headers
```

### Use GDB

```bash
gdb ./myprogram

# Set breakpoint at start
(gdb) break _start

# Run program
(gdb) run

# Step through instructions
(gdb) stepi

# Check registers
(gdb) info registers

# Check memory
(gdb) x/10xg $rsp    # Examine 10 quadwords at stack pointer
```

## Known Constraints and Remaining Gaps

1. **High 8-bit registers with REX**: `ah`, `bh`, `ch`, `dh` cannot be used with R8-R15, SPL/BPL/SIL/DIL, or 64-bit operations (x86_64 architectural limitation). The assembler emits a targeted diagnostic and suggests AL/BL/CL/DL or the low byte of R8-R15.
2. **SSE floating point test completeness**: core SSE instruction support is implemented and tested, but full ISA/operand edge-case coverage is still being expanded.
3. **DWARF debug completeness**: `-g` emits baseline DWARF sections (`.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`) with compile-unit metadata (including compilation directory), per-function metadata, and address/line mapping. The emitter now also guards DWARF buffer writes and fails gracefully with a clear error if internal DWARF buffers are exceeded. Full production-grade DWARF coverage is still a future step.

## Recently Addressed

1. **Include files**: `.include "file"` supports quoted relative/absolute paths and nested include expansion.
2. **Unknown-instruction diagnostics**: parser now reports unknown mnemonics directly (instead of misleading fallback errors).
3. **Control-flow fixups**: symbol hash/table synchronization issues that caused stale branch/call targets have been resolved.
