section .text
global _start
_start:
    mov rcx, 100
    mov rdx, 25
    sub rcx, rdx
    add rcx, 11
    cmp rcx, 86
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 2
    syscall
