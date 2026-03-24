; Test UTF-8 string support
; Demonstrates that UTF-8 encoded text works transparently in db directives

section .data
    ; ASCII text (1 byte per character)
    ascii_text:
        db "Hello, World!", 0
    
    ; Chinese text (3 bytes per character in UTF-8)
    chinese_text:
        db "你好，世界！", 0
    
    ; Russian text (2 bytes per character in UTF-8)
    russian_text:
        db "Привет, мир!", 0
    
    ; Japanese text (3 bytes per character in UTF-8)
    japanese_text:
        db "こんにちは", 0
    
    ; Emoji (4 bytes per character in UTF-8)
    emoji_text:
        db "🎉 Hello 🌍 🚀", 0
    
    ; Mixed content
    mixed_text:
        db "Hello 你好 Привет 🎉", 0
    
    ; Text with escape sequences and UTF-8
    escaped_utf8:
        db "Line1:\n世界\nLine2:\t🌍\n", 0

section .text
global _start

_start:
    ; Exit with code 0
    mov rax, 60     ; sys_exit
    mov rdi, 0      ; exit code
    syscall
