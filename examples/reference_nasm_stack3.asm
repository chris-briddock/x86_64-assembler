section .text
global _start
_start:
    push 0x10
    push 0x20
    pop rax
    pop rbx
    cmp rax, 0x20
    jne fail
    cmp rbx, 0x10
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 13
    syscall
