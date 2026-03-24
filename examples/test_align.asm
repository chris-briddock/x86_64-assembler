; Test alignment directive
; Tests align n directive in both text and data sections

section .text
global _start

_start:
    mov rax, 60     ; sys_exit
    mov rdi, 0      ; exit code
    syscall

section .data
    db 0x01         ; 1 byte - current offset is odd (0x1)
    align 4         ; Align to 4-byte boundary (pads 3 bytes)
aligned_data1:
    dd 0x12345678   ; This should be at 4-byte aligned address

    db 0x02         ; 1 byte - current offset is now 5
    align 8         ; Align to 8-byte boundary (pads 7 bytes from offset 5)
aligned_data2:
    dq 0xDEADBEEFCAFEBABE  ; This should be at 8-byte aligned address

    db 0x03, 0x04   ; 2 bytes - current offset is now 14
    align 4         ; Align to 4-byte boundary (pads 2 bytes from offset 14)
aligned_data3:
    dd 0xAABBCCDD   ; This should be at 4-byte aligned address

; Text section alignment test
section .text
    nop
    nop
    align 16        ; Align to 16-byte boundary in text section
aligned_code:
    nop
    nop
