section .data
a0: dq 1, 2
a1: dq 3, 4

section .text
global _start
_start:
    movaps xmm0, [a0]
    movaps xmm1, [a1]
    addpd xmm0, xmm1
    movaps [a0], xmm0
    mov rax, 60
    xor rdi, rdi
    syscall
