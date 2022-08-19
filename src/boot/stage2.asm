bits 16

section .text

global _start
_start:
    mov si, str_stage2
    call print

    jmp seppuku.loop

print:
    mov bx, 0x0001
    mov ah, 0x0e

    lodsb

    cmp al, 0
    je .exit

    int 10h
    jmp print

.exit:
    ret

seppuku:
    int 18h
.loop:
    jmp .loop

str_stage2:  db "stage2! :)",0xd,0xa,0
