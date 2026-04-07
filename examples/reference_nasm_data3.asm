section .data
v0: dq 5,6,7,8
v1: dq 9

section .text
global _start
_start:
    lea rax, [v0]
    mov rcx, [rax+16]
    mov [rax+24], rcx
    mov rax, 60
    xor rdi, rdi
    syscall
