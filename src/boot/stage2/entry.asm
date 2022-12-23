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
    ;; mov ss, ax

    jmp 0x8:_entry32

;; error:
;;     call puts
;;     mov si, msg_error
;;     call puts
;;     xor ah, ah                    ; Wait for
;;     int 0x16                      ;  keystroke
;;     mov DWORD [BDA_BOOT], 0x1234  ; Do a
;;     jmp 0xf000:0xfff0             ;  warm reboot

;; puts.0:
;;     mov bx, 0x7
;;     mov ah, 0xe
;;     int 0x10
;; puts:
;;     lodsb
;;     test al, al
;;     jne .0

;;     ;; Error return
;; eret:
;;     mov ah, 0x1
;;     stc
;; ret:
;;     ret

;; msg_stage2: db "stage2 woohoo",0
;; msg_error:  db " error",0xd,0xa,0

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
    db 0b10011010
    db 0b11001111
    db 0x00
gdt_end:

bits 32
align 32

extern puts_pe
extern bmain
global _entry32
_entry32:
    cld
    lea esp, _entry32
    push esp

    push msg_bruh
    call puts_pe

    cli
    call bmain

section .rodata
msg_bruh:   db "bruh",0
