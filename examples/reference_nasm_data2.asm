section .data
b0: db 1,2,3,4
w0: dw 0x1122, 0x3344
q0: dq 0x0102030405060708

section .text
global _start
_start:
    lea rax, [q0]
    mov rbx, [rax]
    mov rax, 60
    xor rdi, rdi
    syscall
