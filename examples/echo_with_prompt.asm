; echo_with_prompt.asm - Prompt user for input and echo it back
; Assemble: ./bin/x86_64-asm examples/echo_with_prompt.asm -o echo_with_prompt
; Run: ./echo_with_prompt
; This program displays a prompt, reads input, and echoes it back

section .text
global _start

_start:
    ; Display prompt: "Enter some text: "
    ; sys_write(fd, buf, count)
    mov rax, 1              ; syscall: write
    mov rdi, 1              ; fd: stdout
    lea rsi, [prompt]       ; prompt address
    mov rdx, 18             ; prompt length
    syscall

    ; Read user input
    ; sys_read(fd, buf, count)
    mov rax, 0              ; syscall: read
    mov rdi, 0              ; fd: stdin
    lea rsi, [buffer]       ; buffer address
    mov rdx, 256            ; max bytes to read
    syscall

    ; Check if read was successful
    test rax, rax
    jle .exit               ; exit if nothing read or error

    mov rbx, rax            ; save byte count

    ; Display "You entered: " prefix
    mov rax, 1              ; syscall: write
    mov rdi, 1              ; fd: stdout
    lea rsi, [output_msg]   ; message address
    mov rdx, 13             ; message length
    syscall

    ; Echo the user input
    mov rax, 1              ; syscall: write
    mov rdi, 1              ; fd: stdout
    lea rsi, [buffer]       ; buffer address
    mov rdx, rbx            ; count (bytes read)
    syscall

.exit:
    ; sys_exit(0)
    mov rax, 60             ; syscall: exit
    xor rdi, rdi            ; exit code 0
    syscall

section .data
prompt:
    db 0x45, 0x6e, 0x74, 0x65, 0x72, 0x20       ; "Enter "
    db 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x74       ; "some t"
    db 0x65, 0x78, 0x74, 0x3a, 0x20             ; "ext: "

output_msg:
    db 0x59, 0x6f, 0x75, 0x20, 0x65, 0x6e       ; "You en"
    db 0x74, 0x65, 0x72, 0x65, 0x64, 0x3a       ; "tered:"
    db 0x20                                     ; " "

    ; Buffer stored in .data section (256 bytes = 32 quadwords)
buffer:
    dq $0, $0, $0, $0, $0, $0, $0, $0
    dq $0, $0, $0, $0, $0, $0, $0, $0
    dq $0, $0, $0, $0, $0, $0, $0, $0
    dq $0, $0, $0, $0, $0, $0, $0, $0
