; Test .comm and .lcomm directives
; Demonstrates common symbol declarations for uninitialized data

section .bss
    ; Common symbols (global, can be merged across files)
    .comm buffer, 256          ; 256 bytes, default alignment
    .comm array, 1024, 8       ; 1024 bytes, 8-byte aligned
    .comm big_buffer, 4096, 16 ; 4096 bytes, 16-byte aligned
    
    ; Local common symbols (not global, file-local)
    .lcomm local_buf, 128      ; 128 bytes, default alignment
    .lcomm local_array, 512, 8 ; 512 bytes, 8-byte aligned

section .data
    ; Regular initialized data
    initialized:
        dd 0x12345678

section .text
global _start

_start:
    ; Access common symbols (demonstrates they have addresses)
    lea rax, [buffer]
    lea rbx, [array]
    lea rcx, [local_buf]
    
    ; Exit
    mov rax, 60     ; sys_exit
    mov rdi, 0      ; exit code
    syscall
