; SSE Test Program
; Tests basic SSE move and arithmetic instructions

section .text
global _start

_start:
    ; Reserve stack scratch space and create valid pointers for memory operands
    mov rbp, rsp
    sub rsp, 128
    lea rax, [rsp]
    lea rbx, [rsp + 16]
    lea rcx, [rsp + 32]
    lea rdi, [rsp + 48]
    lea rsi, [rsp + 64]

    ; Test MOVAPS (aligned packed single)
    movaps xmm0, xmm1
    movaps xmm2, [rsp]
    movaps [rsp + 80], xmm3
    
    ; Test MOVUPS (unaligned packed single)
    movups xmm4, xmm5
    movups xmm6, [rdi]
    movups [rsi], xmm7
    
    ; Test MOVSS (scalar single)
    movss xmm0, xmm1
    movss xmm2, [rax]
    movss [rbx], xmm3
    
    ; Test ADDSS (scalar single add)
    addss xmm0, xmm1
    addss xmm2, [rcx]
    
    ; Test SUBSS (scalar single subtract)
    subss xmm4, xmm5
    
    ; Test MULSS (scalar single multiply)
    mulss xmm6, xmm7
    
    ; Test DIVSS (scalar single divide)
    divss xmm8, xmm9
    
    ; Test with high XMM registers
    movaps xmm10, xmm11
    addss xmm12, xmm13
    mulsd xmm14, xmm15
    
    ; Exit
    add rsp, 128
    mov rax, 60
    xor rdi, rdi
    syscall
