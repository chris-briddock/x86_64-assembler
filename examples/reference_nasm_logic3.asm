section .text
global _start
_start:
    mov rcx, 0x1234
    mov rdx, 0x00ff
    and rcx, rdx
    test rcx, rcx
    je fail
    xor rcx, 0x34
    cmp rcx, 0
    jne ok
fail:
    mov rax, 60
    mov rdi, 7
    syscall
ok:
    mov rax, 60
    xor rdi, rdi
    syscall
