; Test file for new assembler features
; This tests: resb, resw, resd, resq, string literals, equ, times

section .data
    ; Test string literals with db
    hello_msg: db "Hello, World!", 10, 0
    test_str: db "Test", 0
    
    ; Test equ directive
    BUFFER_SIZE equ 256
    MAGIC_NUMBER equ 0x12345678
    
    ; Test reserve directives
    buffer: resb 64          ; Reserve 64 bytes
    word_array: resw 10      ; Reserve 10 words (20 bytes)
    dword_array: resd 5      ; Reserve 5 dwords (20 bytes)
    qword_array: resq 4      ; Reserve 4 qwords (32 bytes)

section .text
    global _start

_start:
    ; Test using equ constant
    mov rax, BUFFER_SIZE     ; Should be 256
    mov rbx, MAGIC_NUMBER    ; Should be 0x12345678
    
    ; Test times directive
    times 3 nop              ; Three NOPs
    
    ; Test string address loading
    lea rcx, [hello_msg]     ; Load address of string
    
    ; Exit syscall
    mov rax, 60              ; sys_exit
    xor rdi, rdi             ; exit code 0
    syscall
