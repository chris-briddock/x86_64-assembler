section .text
global _start
_start:
    movdqa xmm8, xmm9
    pcmpeqd xmm8, xmm9
    pcmpgtb xmm10, xmm11
    pand xmm8, xmm10
    por xmm8, xmm10
    mov rax, 60
    xor rdi, rdi
    syscall
