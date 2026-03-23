; SSE Packed Operations Example
; Demonstrates a mixed packed-SSE sequence with stack-backed memory operands.

section .text
global _start

_start:
    ; Reserve scratch space for packed loads/stores.
    sub rsp, 64

    lea rax, [rsp]
    lea rbx, [rsp + 16]
    lea rcx, [rsp + 32]

    ; Packed moves from memory and register paths.
    movdqu xmm0, [rax]
    movdqu xmm1, [rbx]

    ; Packed arithmetic family.
    paddd xmm0, xmm1
    psubd xmm0, [rbx]

    ; Packed compare + logic family.
    pcmpeqd xmm2, xmm2
    pandn xmm2, xmm0
    pxor xmm3, xmm3
    por xmm3, xmm0

    ; Store result vector.
    movdqu [rcx], xmm3

    add rsp, 64

    ; Exit(0)
    mov rax, $60
    xor rdi, rdi
    syscall
