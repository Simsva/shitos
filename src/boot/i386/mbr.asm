[bits 16]
;; cpu 8086

    ;; Memory locations
    %define BDA_BOOT 0x472      ; boot howto flag
    %define PRT_OFF 0x1be       ; partition tabble offset

    ;; Partitions
    %define PRT_NUM 4           ; number of partitions

section .text

global _start
_start:
    cli

    call _realstart
_realstart:
    pop si                                   ; Pop ret addr (*_realstart)
    lea si, [si - (_realstart - _start)]     ; *_start

    push cs
    pop ds

    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00
    mov al, 0x60

    cld
    mov es, ax
    xor di, di
    mov cx, 0x100
    rep movsw

    mov cl, (_highstart - _start)
    push es
    push cx
    retf                        ; Return far to es:cx (*_highstart)

_highstart:
    push cs
    pop ds
    mov si, PRT_OFF
    mov cl, PRT_NUM
.loop:
    test BYTE [si], 0x80
    jnz load
    add si, 16
    loop .loop
    mov si, msg_nobootable
    jmp error

load:
    mov ax, 0x07c0
    mov es, ax
    mov bx, dx                  ; preserve drive number
    mov ax, [si+8]
    mov dx, [si+10]
    call read_sector
    mov si, msg_notfound
    cmp WORD [es:0x1fe], 0xaa55
    jne error
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
    jae .lba_mode               ; LBA if division would overflow

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
    mov si, msg_diskerror
error:
    call puts
    xor ah, ah                    ; Wait for
    int 0x16                      ;  keystroke
    mov DWORD [BDA_BOOT], 0x1234  ; Do a
    jmp 0xf000:0xfff0             ;  warm reboot

puts.0:
    mov bx, 0x7
    mov ah, 0xe
    int 0x10
puts:
    lodsb
    test al, al
    jne .0

    ;; Error return
eret:
    mov ah, 0x1
    stc
ret:
    ret

msg_nobootable: db "no bootable partition found",0
msg_notfound:   db "active partition has no valid boot signature",0
msg_diskerror:  db "disk error",0

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

times PRT_OFF-($-$$) db 0
times 0x40 db 0

boot_magic: dw 0xaa55
