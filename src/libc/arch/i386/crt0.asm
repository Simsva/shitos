bits 32

extern __stdio_init
extern _init
extern main
extern exit

section .text
global _start
_start:
    mov ebp, 0
    push ebp                    ; EIP
    push ebp                    ; EBP
    mov ebp, esp

    ;; preserve arguments
    ;; push ecx                    ; envp
    ;; push edx                    ; envc
    push esi                    ; argv
    push edi                    ; argc

    call _init

    ;; pop edi
    ;; pop esi
    ;; pop edx
    ;; pop ecx

    call __stdio_init
    call main

    push eax
    call exit
