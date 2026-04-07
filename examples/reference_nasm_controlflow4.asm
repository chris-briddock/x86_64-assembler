section .text
global _start
_start:
    mov rax, 1
    test rax, rax
    je fail
    jmp ok
fail:
    mov rax, 60
    mov rdi, 5
    syscall
ok:
    mov rax, 60
    xor rdi, rdi
    syscall
