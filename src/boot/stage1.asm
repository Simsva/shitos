bits 16

section .text

global _start
_start:
    cli

    ;; segment setup
    xor ax, ax
    mov ds, ax
    mov ss, ax

    mov sp, 0x2000

    sti

    ;; save drive reference
    push dx

    ;; relocate MBR to 0x600
    pusha

    mov cx, 0x100
    xor si, si
    mov ds, si
    mov es, si
    mov si, 0x7c00
    mov di, 0x600

    cld

    rep movsw

    popa
    jmp 0:(0x600 + (_start.continue - _start))
.continue:

    ;; print boor message
    mov si, str_booting
    call print

    ;; set si to INT 13h packet
    mov si, disk_packet

    ;; check if LBA is supported
    mov ah, 0x41
    mov bx, 0x55aa
    int 13h

    ;; INT 13h may change dl
    ;; Source: grub i386/pc/boot.S
    pop dx
    push dx

    ;; fall back to CHS if LBA isn't supported
    jc .chs_mode
    cmp bx, 0xaa55
    jne .chs_mode
    and cx, 1
    jz .chs_mode

.lba_mode:
    ;; BIOS call to INT 13h function 0x42 to read sectors from disk into memory
    ;;
    ;; ah = 0x42, dl = drive number, ds:si = segment:offset of disk packet
    ;;
    ;; returns: al = 0x0 on success
    mov ah, 0x42
    ;; reads stage2 (sector 1) into 0x7c00, see disk_packet
    int 13h

    ;; LBA read is not supported so fall back to CHS
    jc .chs_mode

    jmp run_stage2

    ;; TODO: CHS read as fallback in case LBA isn't supported
.chs_mode:
    mov si, str_lba_error
    call print
    jmp seppuku

run_stage2:
    jmp 0:0x7c00

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

    ;; Non-code stuff

str_booting:    db "shitos bootloader",0xd,0xa,0
str_lba_error:  db "LBA not supported...",0xd,0xa,0

    ;; INT 13h packet
disk_packet:
    db 0x10
    db 0x00
num_blocks:
    dw 0x0001
offset:
    dw 0x7c00
segment:
    dw 0x0000
sector:
    dq 0x1

;; times 510 - ($ - _start) db 0
;; dw 0xaa55
