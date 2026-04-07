section .data
v0: dq 1, 2, 3, 4

section .text
global _start
_start:
    lea rax, [v0]
    mov rbx, [rax]
    mov rcx, [rax + rbx * 2 + 8]
    mov [rax + 16], rcx
    mov rax, 60
    xor rdi, rdi
    syscall
