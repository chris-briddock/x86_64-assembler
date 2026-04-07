section .data
veca: dq 1, 2
vecb: dq 3, 4

section .text
global _start
_start:
    movaps xmm0, xmm1
    movups xmm2, [veca]
    movups [vecb], xmm2

    addps xmm0, xmm2
    subps xmm0, xmm2
    mulps xmm0, xmm2
    divps xmm0, xmm2

    movdqa xmm3, xmm4
    movdqu xmm5, [vecb]
    paddb xmm3, xmm5
    paddw xmm3, xmm5
    paddd xmm3, xmm5
    paddq xmm3, xmm5

    psubb xmm3, xmm5
    psubw xmm3, xmm5
    psubd xmm3, xmm5
    psubq xmm3, xmm5

    pcmpeqb xmm6, xmm7
    pcmpeqw xmm6, xmm7
    pcmpeqd xmm6, xmm7
    pcmpeqq xmm6, xmm7

    pcmpgtb xmm6, xmm7
    pcmpgtw xmm6, xmm7
    pcmpgtd xmm6, xmm7
    pcmpgtq xmm6, xmm7

    pand xmm8, xmm9
    pandn xmm8, xmm9
    por xmm8, xmm9
    pxor xmm8, xmm9

    mov rax, $60
    xor rdi, rdi
    syscall
