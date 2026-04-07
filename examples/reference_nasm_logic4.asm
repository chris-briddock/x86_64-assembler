section .text
global _start
_start:
    mov r8, 0xf0
    mov r9, 0x0f
    or r8, r9
    cmp r8, 0xff
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 8
    syscall
