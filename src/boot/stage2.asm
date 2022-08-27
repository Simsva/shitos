bits 16

section .text

global _start
_start:
    ;; read partition entry into part_entry
    xor si, si
    mov ds, si
    mov es, si
    mov si, (0x600 + 446)
.part_loop:
    mov cx, 8
    mov di, part_entry

    cld
    rep movsw
    push si

    ;; check if partition entry is valid
    mov al, BYTE [part_entry.status]
    test al, 0x80
    jz .next_part

    ;; check if partition type is W32 FAT
    mov al, BYTE [part_entry.type]
    cmp al, 0x0b
    je .part_loop_exit

.next_part:
    pop si
    cmp si, (0x600 + 510)
    jl .part_loop

    mov si, str_nobootable
    call print
    jmp seppuku

.part_loop_exit:
    mov si, str_partfound
    call print

    pop si
    mov ax, si
    sub ax, (0x600 + 446)
    shr ax, 4                   ; divide by 16
    call print_dec

    mov si, str_crlf
    call print

; TODO: find kernel in partition $si
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

    ;; prints dl as hex
print_hex:
    mov ah, 0x0e
    mov bx, 0x0001

    mov di, dx
    and di, 0xf0
    shr di, 4
    mov al, BYTE [alphanum + di]
    int 10h

    mov di, dx
    and di, 0x0f
    mov al, BYTE [alphanum + di]
    int 10h

    ret

    ;; prints al as decimal
    ;; NOTE: only handles single digits
print_dec:
    mov ah, 0x0e
    mov bx, 0x0001

    add al, 0x30
    int 10h

    ret

seppuku:
    int 18h
.loop:
    jmp .loop

str_nobootable: db "no bootable partition found",0xd,0xa,0
str_partfound:  db "booting partition ",0
str_crlf:   db 0xd,0xa,0

alphanum:   db "0123456789ABCDEF"

part_entry:
.status:
    db 0x00
.chs_first:
    db 0,0,0
.type:
    db 0x00
.chs_last:
    db 0,0,0
.lba_first:
    dd 0x0
.num_sectors:
    dd 0x0
