section .data
msg: db 'O', 'K', 0
arr: dq 1, 2, 3, 4

section .text
global _start
_start:
    lea rsi, [msg]
    mov rax, [arr]
    add rax, [arr + 8]
    mov [arr + 16], rax
    mov rax, 60
    xor rdi, rdi
    syscall
