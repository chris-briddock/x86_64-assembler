; test_all.asm - Comprehensive test of assembler features

section .text
global _start

_start:
    ; Test register moves
    mov rax, 0x123456789ABCDEF0
    mov rbx, rax
    mov rcx, 0xFF
    mov rdx, rcx

    ; Test 32-bit moves (zero extend)
    mov eax, 0x87654321

    ; Test 16-bit moves
    mov ax, 0xAAAA

    ; Test 8-bit moves
    mov al, 0x11
    mov ah, 0x22

    ; Extended registers
    mov r8, 0x11111111
    mov r9, r8
    mov r10d, 0x22222222
    mov r11w, 0x3333
    mov r12b, 0x44

    ; Arithmetic
    mov rax, 10
    add rax, 5          ; rax = 15
    sub rax, 3          ; rax = 12
    inc rax             ; rax = 13
    dec rax             ; rax = 12

    ; Logic
    mov rax, 0x0F0F
    and rax, 0x00FF     ; rax = 0x000F
    or rax, 0xF000      ; rax = 0xF00F
    xor rax, 0xFF00     ; rax = 0x0F0F
    not rax             ; rax = 0xF0F0

    ; Shifts
    mov rax, 1
    shl rax, 4          ; rax = 16
    shr rax, 2          ; rax = 4

    ; Compare and conditional
    mov rax, 5
    cmp rax, 5
    je equal_test
    mov rdi, 1          ; Exit code 1 if failed
    jmp exit

equal_test:
    ; Test push/pop
    mov rax, 0x12345678
    push rax
    mov rax, 0
    pop rax
    cmp rax, 0x12345678
    je pushpop_ok
    mov rdi, 2
    jmp exit

pushpop_ok:
    ; Test call/ret (using a simple function)
    mov rdi, 42         ; Expected result
    call test_func
    cmp rax, 42
    je call_ok
    mov rdi, 3
    jmp exit

call_ok:
    ; All tests passed
    xor rdi, rdi        ; Exit code 0

exit:
    mov rax, 60         ; sys_exit
    syscall

; Simple function that returns its argument
test_func:
    mov rax, rdi
    ret
