section .text
global _start
_start:
    mov rax, 5
    add rax, 7
    sub rax, 3
    xor rbx, rbx
    cmp rax, 9
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 1
    syscall
