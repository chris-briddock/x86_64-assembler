section .text
global _start
_start:
    mov rax, [here]
    mov rbx, [here + 8]
    mov rax, 60
    xor rdi, rdi
    syscall
here:
    dq 0x1111111111111111
    dq 0x2222222222222222
