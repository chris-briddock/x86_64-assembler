; Test incbin directive - include binary file

section .data
    ; First some regular data
    db 0x01, 0x02, 0x03
    
    ; Include binary file
    incbin "test_data.bin"
    
    ; More regular data after
    db 0x04, 0x05

section .text
global _start

_start:
    mov rax, 60     ; sys_exit
    mov rdi, 0      ; exit code
    syscall
