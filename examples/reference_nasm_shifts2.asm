section .text
global _start
_start:
    mov rax, 0x20
    shl rax, 1
    shr rax, 2
    sar rax, 1
    cmp rax, 8
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 9
    syscall
