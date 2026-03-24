; Test string concatenation
; Demonstrates that adjacent string literals are automatically concatenated

section .data
    ; Basic concatenation
    msg1:
        db "Hello " "World" "!", 0
    
    ; Multiple concatenations
    msg2:
        db "This " "is " "a " "test.", 0
    
    ; Concatenation with escape sequences
    msg3:
        db "Line1\n" "Line2\n" "Line3\n", 0
    
    ; Concatenation with UTF-8
    msg4:
        db "Hello " "你好" " World", 0
    
    ; Building a path
    path:
        db "/usr" "/local" "/bin", 0
    
    ; Empty strings in concatenation
    msg5:
        db "Start" "" "End", 0

section .text
global _start

_start:
    ; Exit with code 0
    mov rax, 60     ; sys_exit
    mov rdi, 0      ; exit code
    syscall
