section .text
global _start

.macro expand_block reg, imm
    mov \reg, \imm
    add \reg, 1
    xor \reg, \reg
    nop
.endm

_start:
    expand_block rax, 1
    expand_block rbx, 2
    expand_block rcx, 3
    expand_block rdx, 4
    expand_block r8, 5
    expand_block r9, 6
    expand_block r10, 7
    expand_block r11, 8
    expand_block r12, 9
    expand_block r13, 10
    expand_block r14, 11
    expand_block r15, 12
    expand_block rax, 13
    expand_block rbx, 14
    expand_block rcx, 15
    expand_block rdx, 16
    expand_block r8, 17
    expand_block r9, 18
    expand_block r10, 19
    expand_block r11, 20
    expand_block r12, 21
    expand_block r13, 22
    expand_block r14, 23
    expand_block r15, 24
    mov rax, $60
    xor rdi, rdi
    syscall
