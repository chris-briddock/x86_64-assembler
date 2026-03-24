; label_arithmetic_equ.asm - Label subtraction in equ expressions
;
; Demonstrates:
; - label arithmetic: NAME equ label1 - label2
; - using equ-defined constants as immediates

section .text
start:
    nop
end:

DELTA equ end - start

global _start
_start:
    mov rax, $60
    mov rdi, DELTA
    syscall
