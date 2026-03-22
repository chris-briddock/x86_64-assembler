; exit.asm - Simple program that exits with status code 42
; Assemble: ./x86_64-asm -o exit exit.asm

section .text
global _start

_start:
    ; sys_exit(status)
    mov rax, 60         ; syscall number for exit (60)
    mov rdi, 42         ; exit code
    syscall             ; make the syscall
