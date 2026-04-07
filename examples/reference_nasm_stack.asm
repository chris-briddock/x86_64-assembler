section .text
global _start
_start:
    push rbp
    mov rbp, rsp
    mov rax, 7
    push rax
    call helper
    pop rbx
    pop rbp
    mov rax, 60
    xor rdi, rdi
    syscall

helper:
    add rax, 1
    ret
