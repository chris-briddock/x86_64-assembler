section .text
global _start
_start:
    xor rcx, rcx
loop_top:
    add rcx, 1
    cmp rcx, 4
    jl loop_top
    cmp rcx, 4
    jne fail
    mov rax, 60
    xor rdi, rdi
    syscall
fail:
    mov rax, 60
    mov rdi, 3
    syscall
