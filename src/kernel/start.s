bits 32
section .text.prologue

extern _main

global _start
_start:
    lea esp, stack
    mov eax, 0xdeadbeef
    push esp
    push eax
    cli
    call _main

section .data
align 32
stack_begin:
    times 0x4000 db 0
stack:
