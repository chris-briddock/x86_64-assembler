section .text
global _start
_start:
    mov rdx, 0
    btc rdx, 1
    bt rdx, 1
    mov rax, 60
    xor rdi, rdi
    syscall
