bits 32

section .text

global _gdt_flush
extern _gdtp
_gdt_flush:
    lgdt [_gdtp]
    mov ax, 0x10                ; entry 2 = ring 0 data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:.1                  ; entry 1 = ring 0 code segment
.1:
    ret
