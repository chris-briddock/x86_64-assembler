section .data
v0: dq 1, 2
v1: dq 3, 4

section .text
global _start
_start:
    movups xmm0, [v0]
    movups xmm1, [v1]
    addps xmm0, xmm1
    movups [v0], xmm0
    movdqa xmm2, xmm3
    pxor xmm2, xmm1
    mov rax, 60
    xor rdi, rdi
    syscall
