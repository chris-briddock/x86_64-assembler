section .data
b0: dq 1, 2
b1: dq 3, 4

section .text
global _start
_start:
    movups xmm2, [b0]
    movups xmm3, [b1]
    mulps xmm2, xmm3
    pxor xmm4, xmm4
    movups [b0], xmm2
    mov rax, 60
    xor rdi, rdi
    syscall
