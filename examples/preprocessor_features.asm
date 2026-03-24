; preprocessor_features.asm - Preprocessor directives showcase
;
; Demonstrates:
; - %define
; - %if / %else / %endif
; - %ifdef / %ifndef
; - %warning
; - %error (left inside inactive branch)

%define USE_FAST_EXIT 1
%define FAST_EXIT_CODE 23

%if USE_FAST_EXIT
%warning Using fast exit path selected by %define
    section .text
    global _start
_start:
    mov rax, $60
    mov rdi, FAST_EXIT_CODE
    syscall
%else
    ; This branch is intentionally inactive. If enabled, it fails preprocessing.
    %error USE_FAST_EXIT disabled unexpectedly
%endif

%ifdef FAST_EXIT_CODE
    ; Symbol is defined above.
%endif

%ifndef MISSING_SYMBOL
    ; Demonstrates negative symbol check.
%endif
