bits 16
;; cpu 8086

section .text

_fakestart:
    cli

    call _realstart
_realstart:
    pop si                                   ; Pop ret addr (*_realstart)
    lea si, [si - (_realstart - _fakestart)] ; *_fakestart

    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00
    mov al, 0x60

    cld
    mov es, ax
    xor di, di
    mov cx, 0x100
    rep movsw

    mov cl, (_highstart - _fakestart)
    push es
    push cx
    retf                        ; Return far to es:cx (*_highstart)

_highstart:
    push cs
    pop ds
    mov si, 446                 ; partitions offset
    mov cl, 4
.loop:
    test BYTE [si], 0x80
    jnz load
    add si, 16
    loop .loop
    mov si, str_nobootable
    jmp print

load:
    mov ax, 0x07c0
    mov es, ax
    mov bx, dx                  ; preserve drive number
    mov ax, [si+8]
    mov dx, [si+10]
    call read_sector
    mov si, str_notfound
    cmp WORD [es:0x1fe], 0xaa55
    jne print
    mov dx, bx
    jmp 0:0x7c00

    ;; Reads sector dx:ax to es:0x0000
    ;; bl = drive number
read_sector:
    ;; Save registers
    pusha                       ; dx (drive number) at bp+12
    push ds

    mov cx, 3                   ; try 3 times

.loop:
    push cx
    mov bp, sp

    mov WORD [sector], ax
    mov WORD [sector + 2], dx

    mov si, disk_packet
    mov ah, 0x08
    mov dl, [bp + 12]

    int 13h
    jc disk_error

    mov ax, 0b111111
    and cx, ax                  ; cx = SPT
    mov al, dh
    inc ax                      ; ax = HPC
    mul cx
    xchg bx, ax                 ; bx = SPT*HPC
    mov dx, WORD [sector + 2]
    cmp dx, bx
    ;; jae .lba_mode               ; LBA if division would overflow
    jmp .lba_mode

    mov ax, WORD [sector]
    div bx                      ; dx = C
    xchg ax, dx                 ; al = H
    div cl                      ; ah = S-1
    mov cl, 2
    xchg ch, dl                 ; dl = 0; ch = low bits of C
    shr dx, cl                  ; dx >>= 2 (dh = 0 if no overflow; dl = high bits of C)
    xchg ah, cl                 ; cl = S - 1; ah = 2
    inc cx                      ; cl = S
    or cl, dl                   ; cl = S | high bits of C
    xchg al, dh                 ; dh = H; al = 0 if no overflow
    ; jz .int13h
.lba_mode:
    ;; Also resets al to 0
    mov ax, 0x4200
.int13h:
    inc ax                      ; al = 1
    les bx, [offset]
    mov dl, [bp + 12]
    push cs
    pop ds
    int 13h

    mov sp, bp
    pop cx
    jnc .done
    loop .loop

.done:
    pop ds
    popa

    jc disk_error
    inc ax
    jnz .ret
    inc dx
.ret:
    ret

disk_error:
    mov si, str_diskerror
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

str_nobootable: db "no bootable partition found",0
str_notfound:   db "hejhej",0
str_diskerror:  db "disk error",0

    ;; INT 13h packet
disk_packet:
    db 0x10
    db 0x00
num_blocks:
    dw 0x0001
offset:
    dw 0x0000
segment:
    dw 0x07c0
sector:
    dq 0x0

times 466 - ($ - $$) db 0
