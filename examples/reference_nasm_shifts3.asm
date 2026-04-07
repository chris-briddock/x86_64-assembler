section .text
global _start
_start:
    mov rcx, 2
    mov rbx, 0x100
    shl rbx, cl
    shr rbx, 3
    cmp rbx, 0x80
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 10
    syscall
