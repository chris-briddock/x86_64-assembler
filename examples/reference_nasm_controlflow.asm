section .text
global _start
_start:
    mov rcx, 4
loop_top:
    dec rcx
    jne loop_top
    call done
    mov rax, 60
    mov rdi, 2
    syscall

done:
    mov rax, 60
    xor rdi, rdi
    syscall
