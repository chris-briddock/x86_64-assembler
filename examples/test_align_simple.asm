; Simple alignment test in text section only

section .text
global _start

_start:
    mov rax, 60     ; sys_exit - 7 bytes
    mov rdi, 0      ; exit code - 7 bytes  
    syscall         ; 2 bytes - total 16 bytes so far
    
    align 8         ; Should be already aligned at 16 bytes
    
aligned_8:
    nop             ; This should be at 0x400010
    nop
    
    align 32        ; Align to 32-byte boundary (pads 14 bytes)
    
aligned_32:
    mov rax, 60     ; sys_exit
    mov rdi, 1      ; exit code 1
    syscall
