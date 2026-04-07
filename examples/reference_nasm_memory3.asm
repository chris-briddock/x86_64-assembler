section .data
buf: dq 1,2,3,4

section .text
global _start
_start:
    lea rbx, [buf]
    mov rcx, [rbx + 8]
    mov rdx, [rbx + rcx*1 - 8]
    mov [rbx + 24], rdx
    mov rax, 60
    xor rdi, rdi
    syscall
