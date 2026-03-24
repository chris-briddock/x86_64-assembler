; Test times directive
section .text
global _start

_start:
    times 5 nop
    mov rax, 60
    syscall
