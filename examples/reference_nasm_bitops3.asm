section .text
global _start
_start:
    mov rcx, 0x80
    bsf rax, rcx
    bsr rbx, rcx
    cmp rax, 7
    jne fail
    cmp rbx, 7
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 11
    syscall
