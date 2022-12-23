bits 32

section .text

global puts_pe
puts_pe:
    mov esi, [esp+4]
    pusha
    mov edi, 0xb8000            ; video memory
    mov ah, 0x0f                ; white on black
.loop:
    lodsb
    test al, al
    je .ret
    mov [edi], ax
    inc edi
    inc edi
    jmp .loop

.ret:
    popa
    ret

global _gdt_flush
extern _gdtp
_gdt_flush:
    lgdt [_gdtp]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:.1
.1:
    ret
