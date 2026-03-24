section .text

; Symbol attribute directives
weak plugin_entry
hidden internal_helper

global _start
_start:
    mov rax, $60
    mov rdi, $0
    syscall

plugin_entry:
    ret

internal_helper:
    ret
