; Test file for character literals
; This demonstrates using single-quoted character literals

section .data
    ; Character literals in db directive
    char_A:     db 'A'              ; Equivalent to db 65
    char_nl:    db '\n'             ; Equivalent to db 10
    char_tab:   db '\t'             ; Equivalent to db 9
    char_zero:  db '\0'             ; Equivalent to db 0
    
    ; String with explicit characters
    msg:        db 'H', 'e', 'l', 'l', 'o', '\n', 0

section .text
    global _start

_start:
    ; Character literals as immediate values
    mov al, 'A'                     ; Load 'A' (65) into al
    mov bl, '\n'                    ; Load newline (10) into bl
    mov cl, '\t'                    ; Load tab (9) into cl
    
    ; Compare with character literal
    cmp al, 'A'                     ; Compare al with 'A'
    je .is_A
    
    jmp .exit
    
.is_A:
    ; Set exit code to 'A' value
    mov dil, al
    
.exit:
    ; Exit syscall
    mov rax, 60
    syscall
