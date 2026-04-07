section .text
global _start
_start:
    enter 16, 0
    leave
    mov rax, 60
    xor rdi, rdi
    syscall
