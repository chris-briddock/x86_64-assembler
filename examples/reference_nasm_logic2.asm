section .text
global _start
_start:
    mov rax, 0xff
    and rax, 0x0f
    or rax, 0x20
    xor rax, 0x01
    cmp rax, 0x2e
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 6
    syscall
