; hello.asm - Write "Hello, World!" to stdout
; This uses raw binary output mode

section .text
global _start

_start:
    ; sys_write(fd, buf, count)
    mov rax, 1          ; syscall: write
    mov rdi, 1          ; fd: stdout
    lea rsi, [msg]      ; buffer address
    mov rdx, msg_len    ; count
    syscall

    ; sys_exit(0)
    mov rax, 60         ; syscall: exit
    xor rdi, rdi        ; exit code 0
    syscall

section .data
msg:
    db 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20  ; "Hello, "
    db 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a  ; "World!\n"
msg_len: dq 14
