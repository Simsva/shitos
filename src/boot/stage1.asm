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

    ;; TODO: relocate MBR to 0x600 and read stage 2 at 0x7c00
    ;;       then locate and read kernel into 0x8000?
.lba_mode:
    xor ax, ax
    mov WORD [si + 4], ax       ; offset

    inc ax
    mov BYTE [si - 1], al       ; mode = 1

    mov WORD [si + 2], ax       ; num_blocks

    ;; the size and the reserved byte
    mov WORD [si], 0x0010

    ;; the absolute address
    mov ebx, DWORD [kernel_sector]
    mov DWORD [si + 8], ebx     ; sector low
    mov ebx, DWORD [kernel_sector_high]
    mov DWORD [si + 12], ebx    ; sector high

    ;; the segment of buffer address
    mov WORD [si + 6], 0x7000   ; segment

    ;; BIOS call to INT 13h function 0x42 to read sectors from disk into memory
    ;;
    ;; ah = 0x42, dl = drive number, ds:si = segment:offset of disk packet
    ;;
    ;; returns: al = 0x0 on success
    mov ah, 0x42
    int 13h

    ;; LBA read is not supported so fall back to CHS
    jc .chs_mode

    mov bx, 0x7000
    jmp .something

    ;; TODO: CHS read as fallback in case LBA isn't supported
.chs_mode:
    mov si, str_lba_error
    call print
    jmp seppuku

.something:
    mov si, 0x7000              ; doesn't actually do anything atm
.rep:
    lodsb
    mov cl, al
    call print_hex
    jmp .rep

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

    ;; prints cl in hex
print_hex:
    mov bx, 0x0001
    mov ah, 0x0e

    mov di, cx
    and di, 0xf0
    shr di, 4
    mov al, BYTE [alphanum_begin + di]
    int 10h

    mov di, cx
    and di, 0x0f
    mov al, BYTE [alphanum_begin + di]
    int 10h

    ret

seppuku:
    int 18h
.loop:
    jmp .loop

    ;; Non-code stuff

str_booting:    db "booting...",0xd,0xa,0
str_lba_error:  db "LBA not supported...",0xd,0xa,0

alphanum_begin: db "0123456789ABCDEF"
alphanum_end:

kernel_sector:      dd 0x1
kernel_sector_high: dd 0x0

    ;; INT 13h packet
mode:
    db 0x00
disk_packet:
    db 0x10
    db 0x00
num_blocks:
    dw 0x0040
offset:
    dw 0x0000
segment:
    dw 0x0000
sector:
    dq 0x0

;; times 510 - ($ - _start) db 0
;; dw 0xaa55
