; arith.asm - Test arithmetic operations
; Computes: ((10 + 5) * 3 - 7) / 2 = 14

section .text
global _start

_start:
    mov rax, 10         ; rax = 10
    add rax, 5          ; rax = 15
    mov rbx, 3
    mul rbx             ; rax = rax * rbx = 45
    sub rax, 7          ; rax = 38
    mov rbx, 2
    xor rdx, rdx        ; Clear rdx for division
    div rbx             ; rax = rax / rbx = 19

    ; sys_exit(rax & 0xFF)
    mov rdi, rax
    and rdi, 0xFF       ; Keep only lower byte for exit code
    mov rax, 60         ; syscall: exit
    syscall
