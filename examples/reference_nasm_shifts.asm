section .text
global _start
_start:
    mov rax, 0x80
    shl rax, 1
    shr rax, 2
    sar rax, 1
    rol rax, 3
    ror rax, 3
    mov rax, 60
    xor rdi, rdi
    syscall
