.macro exit_with_code code
    mov rax, 60
    mov rdi, \code
    syscall
.endm

.macro set_reg_imm reg, val
    mov \reg, $val
.endm

.macro zero_reg reg
    xor \reg, \reg
.endm

.macro push_all
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
.endm

.macro pop_all
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
.endm

.global _start
_start:
    set_reg_imm rax, 42
    set_reg_imm rbx, 100
    zero_reg rdx
    push_all
    mov rax, 999
    mov rbx, 888
    pop_all
    exit_with_code 0
