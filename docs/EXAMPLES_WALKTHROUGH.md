# Example Programs Walkthrough

Detailed explanations of the example programs in the `examples/` directory.

## Table of Contents

1. [exit.asm - Simplest Program](#exitasm---simplest-program)
2. [arith.asm - Arithmetic Operations](#arithasm---arithmetic-operations)
3. [loop.asm - Loops and Conditionals](#loopasm---loops-and-conditionals)
4. [hello.asm - System Calls and Data](#helloasm---system-calls-and-data)
5. [simple.asm - Function Calls](#simpleasm---function-calls)
6. [test_all.asm - Comprehensive Test](#test_allasm---comprehensive-test)

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

## Summary of Examples

| Example | Concepts Covered |
| :--- | :--- |
| exit.asm | Basic program structure, syscalls |
| arith.asm | Arithmetic operations, immediates |
| loop.asm | Labels, loops, conditional jumps |
| hello.asm | Data sections, strings, I/O syscalls |
| simple.asm | Functions, call/ret, arguments |
| test_all.asm | Memory operations, comprehensiveness |

## Next Steps

Try modifying these examples:

1. Change the exit codes
2. Modify the loop counter
3. Add more arithmetic operations
4. Change the string in hello.asm
5. Create your own functions

For more advanced topics, see [ASSEMBLY_REFERENCE.md](ASSEMBLY_REFERENCE.md).
