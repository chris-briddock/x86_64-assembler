section .text
global _start
_start:
    mov r8, 1
    mov r9, 2
    add r8, r9
    mov r10, r8
    cmp r10, 3
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 3
    syscall
