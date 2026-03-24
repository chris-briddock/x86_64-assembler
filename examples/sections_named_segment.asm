; sections_named_segment.asm - Named section prefixes and segment alias
;
; Demonstrates:
; - section .data.* and section .text.* named variants
; - segment alias for section switching

section .data.runtime
    exit_code: dq $17

segment .text.hot
global _start
_start:
    ; Load value from named data section and exit with that code.
    mov rdi, [exit_code]
    mov rax, $60
    syscall
