bits 16
;; cpu 8086

section .text

global _start
_start:
    mov ax, 0x07c0
    mov ds, ax
    mov si, str_bruh

print:
    mov bx, 0x0001
    mov ah, 0x0e

    lodsb

    cmp al, 0
    je seppuku

    int 10h
    jmp print
seppuku:
    hlt
    jmp seppuku

str_bruh:   db "stage 1 :)",0

times 510 - ($ - $$) db 0
dw 0xaa55
