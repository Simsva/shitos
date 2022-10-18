bits 16
;; cpu 8086

section .text

global _start
_start:
    jmp _realstart
    nop

                 db 'SHTOS1.0'
                 dw 512
bpb_spercluster: db 2
bpb_sreserved:   dw 6270
bpb_fats:        db 2
                 dw 0
                 dw 0
                 db 0xF8
                 dw 0
bpb_sectors:     dw 63
bpb_heads:       dw 255
bpb_startlba:    dd 0
                 dd 253952
bpb_sperfat:     dd 961
                 dw 0x0000
                 dw 0x0000
bpb_rootdir:     dd 2
                 dw 1
                 dw 6
temp_fatsector:  times 4 db 0x00
temp_datastart:  times 8 db 0x00
bpb_drivenumber: db 0x80
                 db 0x00
                 db 0x29
                 dd 0x12345678
                 db 'SHITOS     '
                 db 'FAT32   '

    ;; FAT32 VBR "high start"
_realstart:
;;     int 12h
;;     cli

;;     mov cl, 6
;;     shl ax, cl
;;     dec ah                      ; reserve 4 KiB

;;     mov es, ax
;;     mov ss, ax
;;     mov sp, 0x1000

;;     cld
;;     mov si, 0x7c00
;;     xor di, di
;;     mov ds, di
;;     mov cx, 0x100
;;     rep movsw                   ; copy VBR to
;;     mov ds, ax

;;     sti

;;     mov cl, _highstart-_start ; offset due to BPB
;;     push es
;;     push cx
;;     retf
    mov ax, 0x7c0
    mov es, ax
    mov ds, ax
    mov cx, _highstart-_start
    push es
    push cx
    retf
_highstart:
    mov BYTE [bpb_drivenumber], dl
    xor di, di
    mov es, di
    mov ah, 0x8
    int 13h

    mov si, err_geometry
    jc print

    movzx dx, dh
    inc dx
    mov WORD [bpb_heads], dx

    and cx, 0x3f
    mov WORD [bpb_sectors], cx

    mov ax, cs
    add ax, 0x20
    mov es, ax

    mov eax, 2
    cdq                         ; sign extend eax to edx:eax
    ;; FIXME: Add offset of FAT partition

test:
    call readsector
    mov ax, [sig]
    cmp ax, [sig+0x200]
    mov si, err_corrupt
    je findfile
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

readsector:
    ;; edx:eax - sector to read
    ;; es:0    - dest
    ;; trashes everything, assumes cs=ds
    mov bp, sp
    ;; add eax, DWORD [bpb_startlba]
    ;; adc edx, 0

    ;; construct disk_packet on stack
    ;; push edx
    ;; push eax
    ;; push es
    ;; push DWORD 1
    ;; push WORD 16
    ;; mov si, sp
    mov DWORD [dp.sector+4], edx
    mov DWORD [dp.sector], eax
    mov WORD [dp.segment], es
    mov si, dp

    ;; TODO: fallback to CHS
.lba_mode:
    mov ax, 0x4200
.int13:
    inc ax,
    xor bx, bx
    mov dl, BYTE [bpb_drivenumber]
    int 13h
    jc .error
    mov sp, bp
    ret
.error:
    mov si, err_bruh
    jmp print

err_geometry:   db "BIOS geometry error",0
err_corrupt:    db "Corrupt boot code",0
err_bruh:       db "bruh",0
str_succ:       db "success",0

dp:
    db 0x10
    db 0x00
.num_blocks:
    dw 0x0001
.offset:
    dw 0x0000
.segment:
    dw 0x07c0
.sector:
    dq 0x0

times 0x1fe - ($-$$) db 0
sig:    dw 0xaa55

 ;; sector 1 (FSInfo)
dd 0x41615252

findfile:
    mov si, err_bruh
    jmp seppuku

times 0x3e2 - ($-$$) db 0
dd 0x61417272
