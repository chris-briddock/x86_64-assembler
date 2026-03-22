; loop.asm - Test loop and conditional jumps
; Counts down from 5 to 0 and exits with final count

section .text
global _start

_start:
    mov rcx, 5          ; Initialize counter

loop_start:
    cmp rcx, 0          ; Compare counter with 0
    je loop_done        ; If zero, exit loop
    dec rcx             ; Decrement counter
    jmp loop_start      ; Continue looping

loop_done:
    ; sys_exit(rcx)
    mov rax, 60         ; syscall: exit
    mov rdi, rcx        ; exit code (should be 0)
    syscall
