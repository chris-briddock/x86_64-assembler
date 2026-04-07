section .text
global _start
_start:
    mov rax, 0x1234
    bt rax, 4
    bts rax, 5
    btr rax, 1
    btc rax, 3
    bsf rcx, rax
    bsr rdx, rax
    mov rax, 60
    xor rdi, rdi
    syscall
