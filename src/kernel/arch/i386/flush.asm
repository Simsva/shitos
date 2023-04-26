bits 32

section .text

global _gdt_flush
extern _gdtp
_gdt_flush:
    lgdt [_gdtp]
    mov ax, 0x10                ; ring 0 DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:.1                  ; ring 0 CS
.1:
    ret

global _tss_flush
_tss_flush:
    mov ax, 0x28 | 0x03
    ltr ax
    ret

global _idt_load
extern _idtp
_idt_load:
    lidt [_idtp]
    ret
