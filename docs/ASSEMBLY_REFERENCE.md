# x86_64 Assembly Language Reference

This document provides a comprehensive reference for the assembly syntax
supported by this assembler.

## Table of Contents

1. [Basic Syntax](#basic-syntax)
2. [Registers](#registers)
3. [Instructions](#instructions)
4. [Addressing Modes](#addressing-modes)
5. [Sections](#sections)
6. [Labels and Symbols](#labels-and-symbols)
7. [Data Directives](#data-directives)

## Basic Syntax

### Program Structure

An assembly program consists of:

1. **Sections** - Define memory regions (.text, .data, .bss, .rodata)
2. **Labels** - Named locations in code or data
3. **Instructions** - CPU operations
4. **Directives** - Assembler commands

### Comments

Use semicolon (`;`) for comments. Everything after `;` to end of line is ignored.

```asm
mov rax, $42    ; This is a comment
```

### Case Sensitivity

Instructions and register names are case-insensitive:

```asm
MOV RAX, $42    ; Same as: mov rax, $42
```

## Registers

### 64-bit General Purpose Registers

| Register | Description | Callee-Saved |
| :--- | :--- | :--- |
| `rax` | Accumulator, return value | No |
| `rbx` | Base register | Yes |
| `rcx` | Counter (4th argument) | No |
| `rdx` | Data (3rd argument) | No |
| `rsi` | Source index (2nd argument) | No |
| `rdi` | Destination index (1st argument) | No |
| `rbp` | Base pointer | Yes |
| `rsp` | Stack pointer | Yes |
| `r8-r15` | Extended registers | r12-r15: Yes |

### 32-bit Registers

Lower 32 bits of 64-bit registers: `eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`,
`ebp`, `esp`, `r8d-r15d`

### 16-bit Registers

Lower 16 bits: `ax`, `bx`, `cx`, `dx`, `si`, `di`, `bp`, `sp`, `r8w-r15w`

### 8-bit Registers

Lower 8 bits: `al`, `bl`, `cl`, `dl`, `sil`, `dil`, `bpl`, `spl`, `r8b-r15b`

High 8 bits: `ah`, `bh`, `ch`, `dh`

**Note on High 8-bit Registers:** Due to x86_64 architecture constraints, AH/BH/CH/DH cannot be used with:

- R8-R15 registers
- SPL/BPL/SIL/DIL registers
- 64-bit operations requiring REX prefix

Use AL/BL/CL/DL or the low byte of R8-R15 (R8B-R15B) instead for compatibility.

## Instructions

### Non-SSE Scalar Support Matrix

This matrix lists the currently supported non-SSE scalar instructions by family.

| Family | Supported Instructions |
| :--- | :--- |
| Data movement | `mov`, `movzx`, `movsx`, `lea`, `push`, `pop`, `xchg`, `bswap` |
| Integer arithmetic/logic | `add`, `adc`, `sub`, `sbb`, `inc`, `dec`, `neg`, `not`, `cmp`, `xor`, `or`, `and`, `test`, `mul`, `imul`, `div`, `idiv` |
| Shifts/rotates | `shl`, `shr`, `sal`, `sar`, `rol`, `ror`, `rcl`, `rcr`, `shld`, `shrd` |
| Bit test/scan | `bt`, `bts`, `btr`, `btc`, `bsf`, `bsr` |
| Control flow | `jmp`, `ja`, `jae`, `jb`, `jbe`, `je`, `jg`, `jge`, `jl`, `jle`, `jne`, `jnz`, `jno`, `jnp`, `jns`, `jo`, `jp`, `js`, `call`, `ret` |
| Conditional move/set | `cmova`, `cmovae`, `cmovb`, `cmovbe`, `cmove`, `cmovg`, `cmovge`, `cmovl`, `cmovle`, `cmovne`, `cmovno`, `cmovnp`, `cmovns`, `cmovo`, `cmovp`, `cmovs`, `seta`, `setae`, `setb`, `setbe`, `sete`, `setg`, `setge`, `setl`, `setle`, `setne`, `setno`, `setnp`, `setns`, `seto`, `setp`, `sets` |
| System/flags | `int`, `syscall`, `sysret`, `nop`, `hlt`, `clc`, `stc`, `cmc`, `cld`, `std`, `cli`, `sti` |
| Sign-extension helpers | `cbw`, `cwde`, `cdqe`, `cwd`, `cdq`, `cqo` |
| Stack frame | `leave`, `enter` |
| String move | `movsb`, `movsw`, `movsd`, `movsq` |

### Data Movement

#### MOV - Move Data

```asm
mov rax, $42           ; Immediate to register
mov rax, rbx           ; Register to register
mov rax, [mem]         ; Memory to register
mov [mem], rax         ; Register to memory
mov rax, label         ; Load address (RIP-relative)
```

#### LEA - Load Effective Address

```asm
lea rax, [rbx + 8]     ; Compute address, not dereference
lea rsi, [label]       ; Get address of label
```

#### PUSH / POP - Stack Operations

```asm
push rax               ; Push register onto stack
push $42               ; Push immediate (sign-extended)
pop rax                ; Pop from stack to register
```

### Arithmetic Instructions

#### ADD / SUB

```asm
add rax, $10           ; Add immediate
add rax, rbx           ; Add register
add rax, [mem]         ; Add from memory
sub rax, $5            ; Subtract immediate
```

#### INC / DEC

```asm
inc rax                ; Increment by 1
dec rbx                ; Decrement by 1
```

#### NEG / NOT

```asm
neg rax                ; Negate (two's complement)
not rax                ; Bitwise NOT
```

#### Bitwise Operations

```asm
and rax, $0xFF         ; Bitwise AND
or rax, $0x10          ; Bitwise OR
xor rax, rax           ; Bitwise XOR (clears register)
```

#### CMP - Compare

```asm
cmp rax, $10           ; Compare with immediate
cmp rax, rbx           ; Compare with register
```

### Control Flow

#### JMP - Unconditional Jump

```asm
jmp label              ; Direct jump
jmp rax                ; Indirect jump (register)
```

#### Conditional Jumps Reference

| Instruction | Condition | Description |
| :--- | :--- | :--- |
| `je` | Equal (ZF=1) | Jump if equal |
| `jne` | Not Equal (ZF=0) | Jump if not equal |
| `jnz` | Not Zero (ZF=0) | Synonym for jne |
| `ja` | Above (CF=0 && ZF=0) | Unsigned above |
| `jae` | Above or Equal (CF=0) | Unsigned above/equal |
| `jb` | Below (CF=1) | Unsigned below |
| `jbe` | Below or Equal (CF=1 or ZF=1) | Unsigned below/equal |
| `jg` | Greater (ZF=0 && SF=OF) | Signed greater |
| `jge` | Greater or Equal (SF=OF) | Signed greater/equal |
| `jl` | Less (SF!=OF) | Signed less |
| `jle` | Less or Equal (ZF=1 or SF!=OF) | Signed less/equal |
| `jo` | Overflow (OF=1) | Jump if overflow |
| `jno` | No Overflow (OF=0) | Jump if no overflow |
| `js` | Sign (SF=1) | Jump if negative |
| `jns` | No Sign (SF=0) | Jump if non-negative |
| `jp` | Parity (PF=1) | Jump if parity even |
| `jnp` | No Parity (PF=0) | Jump if parity odd |

```asm
cmp rax, $5
je equal_label         ; Jump if rax == 5
jne not_equal          ; Jump if rax != 5
jnz not_zero           ; Same as jne (synonym)
```

#### CALL / RET - Function Calls

```asm
call function_name     ; Call subroutine
ret                    ; Return from subroutine
```

### System Instructions

#### SYSCALL

```asm
mov rax, $60           ; System call number (exit)
mov rdi, $0            ; First argument (exit code)
syscall                ; Make system call
```

Common Linux system calls:

- `0` - sys_read
- `1` - sys_write
- `60` - sys_exit

### String Instructions

String instructions operate on memory pointed to by RSI (source) and RDI (destination), automatically incrementing or decrementing based on the direction flag.

#### MOVS - Move String

```asm
movsb                  ; Move byte from [RSI] to [RDI]
movsw                  ; Move word (2 bytes)
movsd                  ; Move doubleword (4 bytes)
movsq                  ; Move quadword (8 bytes)
```

**Example - Copy memory:**

```asm
section .data
src: db $48, $65, $6C, $6C, $6F    ; "Hello"
dst: db $0, $0, $0, $0, $0

section .text
global _start
_start:
    lea rsi, [src]       ; Source address
    lea rdi, [dst]       ; Destination address
    mov rcx, $5          ; Counter
    cld                  ; Clear direction flag (increment)
    
copy_loop:
    movsb                ; Copy byte, increment RSI and RDI
    dec rcx
    jnz copy_loop
    
    mov rax, $60
    xor rdi, rdi
    syscall
```

**Notes:**

- Use `cld` to increment RSI/RDI (forward copy)
- Use `std` to decrement RSI/RDI (backward copy)
- RCX is typically used as a counter with `rep` prefix (not yet supported)

### SSE Support Matrix

The assembler supports a baseline SSE subset. This matrix documents currently supported forms and where coverage exists in tests.

For the expanded roadmap matrix (including addressing/register-band depth and gap tracking), see [SSE Coverage Matrix](SSE_COVERAGE_MATRIX.md).

#### Move Instructions

| Instruction | Supported Forms | Representative Tests |
| :--- | :--- | :--- |
| `movaps` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movaps_xmm0_xmm1`, `test_movaps_mem_xmm15`, `test_movaps_xmm9_r12_r13_scale8_disp32` |
| `movups` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movups_xmm0_mem`, `test_movups_xmm2_rsp_disp8`, `test_movups_xmm1_rip_disp32` |
| `movss` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movss_xmm1_xmm2`, `test_integration_sse_smoke` |
| `movapd` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movapd_xmm8_complex_addr` |
| `movupd` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | Encoded by same move path as `movups` with `0x66` prefix |
| `movsd` (SSE scalar) | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movsd_mem_xmm1_rbp` |

#### Arithmetic Instructions

| Instruction Family | Supported Forms | Representative Tests |
| :--- | :--- | :--- |
| `addss/addps/addsd/addpd` | `xmm, xmm`; `xmm, [mem]` | `test_addss_xmm0_xmm1`, `test_integration_sse_mixed_controlflow` |
| `subss/subps/subsd/subpd` | `xmm, xmm`; `xmm, [mem]` | `test_integration_sse_smoke` |
| `mulss/mulps/mulsd/mulpd` | `xmm, xmm`; `xmm, [mem]` | `test_mulsd_xmm8_xmm9` |
| `divss/divps/divsd/divpd` | `xmm, xmm`; `xmm, [mem]` | `test_divps_xmm4_mem` |

#### Packed Integer / Compare / Logic Instructions

| Instruction Family | Supported Forms | Representative Tests |
| :--- | :--- | :--- |
| `movdqa/movdqu` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | `test_movdqa_xmm0_xmm1`, `test_movdqu_xmm8_mem` |
| `paddb/paddw/paddd/paddq` | `xmm, xmm`; `xmm, [mem]` | `test_paddb_xmm1_xmm2`, `test_paddw_xmm0_mem` |
| `psubb/psubw/psubd/psubq` | `xmm, xmm`; `xmm, [mem]` | Covered by same packed arithmetic encoder path |
| `pcmpeqb/pcmpeqw/pcmpeqd/pcmpeqq` | `xmm, xmm`; `xmm, [mem]` | `test_pcmpeqd_xmm7_xmm15`, `test_pcmpeqq_xmm1_xmm2` |
| `pcmpgtb/pcmpgtw/pcmpgtd/pcmpgtq` | `xmm, xmm`; `xmm, [mem]` | `test_pcmpgtb_xmm3_mem_disp`, `test_pcmpgtq_xmm8_mem_disp` |
| `pand/pandn/por/pxor` | `xmm, xmm`; `xmm, [mem]` | `test_pand_xmm4_xmm5`, `test_por_xmm2_mem` |

`pcmpeqq` and `pcmpgtq` use the `0F 38` opcode map and are now explicitly regression-tested.

#### Current SSE Diagnostic Guarantees

| Invalid Form | Expected Diagnostic (substring) | Representative Tests |
| :--- | :--- | :--- |
| No XMM operand in SSE move | `requires at least one XMM operand` | `test_sse_mov_without_xmm_rejected` |
| SSE move with memory destination and memory source | `cannot have two memory operands` | `test_sse_mov_mem_to_mem_without_xmm_rejected` |
| Memory destination with non-XMM source | `memory destination requires XMM source` | `test_sse_mov_mem_dst_imm_src_rejected` |
| XMM destination with unsupported source type | `XMM destination requires XMM or memory source` | `test_sse_mov_xmm_dst_imm_src_rejected` |
| SSE arithmetic destination not XMM | `destination must be XMM register` | `test_sse_arith_dst_not_xmm_rejected` |
| SSE arithmetic source invalid type | `source must be XMM or memory` | `test_sse_arith_imm_src_rejected` |
| XMM reg used with wrong size metadata | `must use xmm register size` | `test_sse_mov_xmm_size_mismatch_rejected`, `test_sse_arith_xmm_src_size_mismatch_rejected` |

## Addressing Modes

### Direct Register

```asm
mov rax, rbx           ; rbx -> rax
```

### Immediate

```asm
mov rax, $42           ; 42 -> rax
mov rax, $0xFF         ; Hex immediate
mov rax, $-5           ; Negative immediate
```

### Direct Memory

```asm
mov rax, [label]       ; Load from label address
mov [label], rbx       ; Store to label address
```

### Register Indirect

```asm
mov rax, [rbx]         ; Load from address in rbx
mov [rbx], rax         ; Store to address in rbx
```

### Displaced Addressing

```asm
mov rax, [rbx + 8]     ; rbx + 8
mov rax, [rbx - 8]     ; rbx - 8
mov rax, [rbx + $10]   ; rbx + 16 (hex)
```

### Scaled Index Addressing

```asm
mov rax, [rbx + rcx*2]   ; rbx + rcx * 2
mov rax, [rbx + rcx*4]   ; rbx + rcx * 4
mov rax, [rbx + rcx*8]   ; rbx + rcx * 8
mov rax, [rbx + rsi*4 + 16]  ; base + index * scale + displacement
```

Scale factors: 1, 2, 4, 8

### RIP-Relative

Used for position-independent code:

```asm
lea rax, [rip + label]     ; Address of label relative to IP
mov rbx, [rip + data_var]  ; Load from data section
```

## Sections

### TEXT Section

Contains executable code. Read-only in running program.

```asm
section .text
global _start
_start:
    ; Your code here
```

### DATA Section

Contains initialized read-write data.

```asm
section .data
message: db $48, $65, $6C, $6C, $6F  ; "Hello"
counter: dq $0                        ; 64-bit variable
```

### BSS Section

Contains uninitialized read-write data (zero-initialized).

```asm
section .bss
buffer: resb $100     ; Reserve 100 bytes
array:  resq $10      ; Reserve 10 quadwords (80 bytes)
```

### RODATA Section

Contains read-only data.

```asm
section .rodata
pi:     dq $3.14159   ; Read-only constant
msg:    db "Hello"    ; Read-only string
```

## Labels and Symbols

### Defining Labels

```asm
my_label:              ; Label definition
    mov rax, $42

another_label:         ; Another label
    nop
```

### Using Labels

```asm
jmp my_label           ; Jump to label
call function          ; Call function at label
mov rax, [my_data]     ; Access data at label
```

### Symbol References

```asm
section .data
msg: db "Hello"
len: equ $5            ; Constant definition

section .text
mov rdx, $len          ; Use constant
```

## Data Directives

### DB - Define Byte

```asm
db $48                 ; Single byte
db $48, $65, $6C       ; Multiple bytes
```

### DW - Define Word (2 bytes)

```asm
dw $1234               ; 16-bit value
```

### DD - Define Doubleword (4 bytes)

```asm
dd $12345678           ; 32-bit value
```

### DQ - Define Quadword (8 bytes)

```asm
dq $123456789ABCDEF0   ; 64-bit value
```

### EQU - Define Constant

```asm
buffer_size: equ $1024    ; Constant, no storage
```

## Complete Example

```asm
; Example: Calculate sum of 1 to 10
section .data
result: dq $0           ; Store result here

section .text
global _start
_start:
    mov rcx, $10        ; Counter: 10 down to 1
    mov rax, $0         ; Accumulator

sum_loop:
    add rax, rcx        ; Add counter to sum
    dec rcx             ; Decrement counter
    jnz sum_loop        ; Loop until counter is 0

    mov [result], rax   ; Store result

    ; Exit with sum as exit code
    mov rdi, rax        ; Exit code = sum (55)
    mov rax, $60        ; sys_exit
    syscall
```

## Limitations and Notes

1. **Linux x86_64 only** - ELF64 output format
2. **High 8-bit register constraint** - `ah`/`bh`/`ch`/`dh` cannot be encoded with REX-prefixed operands (architectural rule)
3. **SSE coverage is broad but not exhaustive** - core XMM move/arithmetic paths are implemented and tested, but full edge-case matrix is still incomplete
4. **DWARF is baseline-level with verified structure** - `-g` emits `.debug_info`, `.debug_abbrev`, `.debug_line`, and `.debug_str` with compile-unit metadata (including `DW_AT_comp_dir`), per-function DIEs, line program end-sequence markers, and fail-fast buffer-overflow protection during DWARF section construction
5. **Macro/include support exists** - `.macro`/`.endm` and `.include` are available; prefer tests/examples for advanced edge cases

## See Also

- [README.md](../README.md) - Project overview and quick start
- Intel 64 and IA-32 Architectures Software Developer's Manual
- System V AMD64 ABI Specification
