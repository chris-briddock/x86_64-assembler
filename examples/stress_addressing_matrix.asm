section .data
buf: dq 0, 1, 2, 3

section .text
global _start
_start:
    mov rax, [buf]
    mov rbx, [buf + 8]
    mov rcx, [rax + rbx * 2 + 16]
    mov rdx, [rax + rbx * 4 + 32]
    mov r8,  [rax + rbx * 8 + 64]

    lea r9,  [rax + rbx * 1 + 8]
    lea r10, [rax + rbx * 2 + 16]
    lea r11, [rax + rbx * 4 + 32]
    lea r12, [rax + rbx * 8 + 64]

    add rax, [r9 + r10 * 2 + 16]
    sub rbx, [r9 + r10 * 4 + 32]
    xor rcx, [r9 + r10 * 8 + 64]

    mov [buf], rax
    mov [buf + 8], rbx
    mov [buf + 16], rcx

    mov rax, $60
    xor rdi, rdi
    syscall
