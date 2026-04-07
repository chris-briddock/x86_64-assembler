section .text
global _start
_start:
    mov rax, 1
    mov rbx, 8
    shld rbx, rax, 1
    shrd rbx, rax, 1
    mov rax, 60
    xor rdi, rdi
    syscall
