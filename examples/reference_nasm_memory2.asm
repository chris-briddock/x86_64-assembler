section .data
arr: dq 10,20,30,40

section .text
global _start
_start:
    lea rbx, [arr]
    mov rax, [rbx + 8]
    mov rcx, [rbx + rax*1 - 8]
    mov [rbx + 24], rcx
    mov rax, 60
    xor rdi, rdi
    syscall
