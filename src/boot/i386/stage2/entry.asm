bits 16

    ;; Memory locations
    %define MEM_STG1 0x700      ; location of relocated stage1
    %define MEM_ARG 0x900       ; location of dx
    %define MEM_ORG 0x9000      ; origin
    %define BDA_BOOT 0x472      ; boot howto flag

section .text.prologue

global _start
_start:
    ;; enable PE flag
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;; jump to flush prefetch queue
    jmp .flush
.flush:
    lidt [idt]
    lgdt [gdtp]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x8:_entry32

    ;; IDT
align 16
idt:
    dw 0
    dq 0

    ;; GDT
align 16
gdtp:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 16
gdt_start:
gdt_null:
    dq 0
gdt_code_segment:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0b10011010
    db 0b11001111
    db 0x00
gdt_data_segment:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0b10010010
    db 0b11001111
    db 0x00
gdt_end:

bits 32
align 32

extern bmain
global _entry32
_entry32:
    lea esp, stack
    push esp

    cld                         ; SysV ABI requires the DF flag to be cleared
                                ; on function entry
    cli
    call bmain

section .bss
    resb 0x2000                 ; 8 KiB
stack:
