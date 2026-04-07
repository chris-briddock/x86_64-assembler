section .text
global _start
_start:
    mov rax, 42
    push rax
    pop rbx
    cmp rbx, 42
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 12
    syscall
