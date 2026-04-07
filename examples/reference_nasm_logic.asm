section .text
global _start
_start:
    mov rax, 0x55
    mov rbx, 0x0f
    and rax, rbx
    or rax, 0x30
    xor rax, 0x10
    test rax, rax
    jne done
    mov rax, 60
    mov rdi, 1
    syscall
done:
    mov rax, 60
    xor rdi, rdi
    syscall
