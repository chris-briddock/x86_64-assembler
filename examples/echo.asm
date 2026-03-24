; echo.asm - Read user input and echo it back to stdout
; Assemble: ./bin/x86_64-asm examples/echo.asm -o echo
; Run: ./echo
; This program reads up to 256 bytes from stdin and writes them to stdout

section .text
global _start

_start:
    ; sys_read(fd, buf, count)
    mov rax, 0          ; syscall: read
    mov rdi, 0          ; fd: stdin
    lea rsi, [buffer]   ; buffer address
    mov rdx, 256        ; max bytes to read
    syscall

    ; rax now contains the number of bytes read
    ; if rax <= 0, nothing was read or error occurred
    test rax, rax
    jle .exit           ; exit if nothing read or error

    mov rbx, rax        ; save byte count for write syscall

    ; sys_write(fd, buf, count)
    mov rax, 1          ; syscall: write
    mov rdi, 1          ; fd: stdout
    lea rsi, [buffer]   ; buffer address
    mov rdx, rbx        ; count (bytes read)
    syscall

.exit:
    ; sys_exit(0)
    mov rax, 60         ; syscall: exit
    xor rdi, rdi        ; exit code 0
    syscall

section .data
    ; Buffer stored in .data section (256 bytes = 32 quadwords)
    ; Using dq with zeros to reserve space
    buffer: dq $0, $0, $0, $0, $0, $0, $0, $0
            dq $0, $0, $0, $0, $0, $0, $0, $0
            dq $0, $0, $0, $0, $0, $0, $0, $0
            dq $0, $0, $0, $0, $0, $0, $0, $0
