section .text
global _start
_start:
    call helper
    cmp rax, 7
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
helper:
    mov rax, 7
    ret
fail:
    mov rax, 60
    mov rdi, 4
    syscall
