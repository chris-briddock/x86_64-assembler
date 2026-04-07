section .text
global _start
_start:
    mov rax, 10
    mov rbx, 3
    imul rax, rbx
    add rax, 7
    sub rax, 5
    cmp rax, 32
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 1
    syscall
