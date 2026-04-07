section .text
global _start
_start:
    mov rax, 0
    bts rax, 5
    bt rax, 5
    btr rax, 5
    bt rax, 5
    mov rax, 60
    xor rdi, rdi
    syscall
