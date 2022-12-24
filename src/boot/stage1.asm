[bits 16]
;; cpu 8086

    ;; Memory locations
    %define MEM_REL 0x700       ; stage1 relocation address
    %define MEM_ARG 0x900       ; location of dx
    %define MEM_ORG 0x7c00      ; origin
    %define MEM_BUF 0x8e00      ; nread load area
    %define MEM_STG2 0x9000     ; stage2 start
    %define BDA_BOOT 0x472      ; boot howto flag

    ;; Partitions
    %define PRT_OFF 0x1be       ; partition tabble offset
    %define PRT_NUM 4           ; number of partitions
    %define PRT_TYP 0x13        ; partition type

    ;; Misc constants
    %define NSECT 0x20          ; number of sectors of stage2 to read

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

    ;; Trampoline to call read from stage2
    ;; Copied from FreeBSD
    ;;
    ;; cx:ax   - LBA to read in
    ;; es:(bx) - buffer to read data into
    ;; dl      - drive to read from
    ;; dh      - num sectors to read
xread:
    push ss
    pop ds
.1:
    push DWORD 0x0              ; Starting
    push cx                     ;  LBA
    push ax                     ;  block
    push es                     ; Transfer
    push bx                     ;  buffer
    xor ax, ax
    mov al, dh
    push ax                     ; Number of blocks
    push WORD 0x0010            ; 0 + packet size
    mov bp, sp
    call read
    lea sp, [bp + 0x10]
    ret

_realstart:
    cld                         ; String ops inc
    xor cx, cx
    mov es, cx
    mov ds, cx
    mov ss, cx
    mov sp, _start

    ;; Relocate code to MEM_REL
    mov si, sp
    mov di, MEM_REL
    inc ch                      ; 0x100
    rep movsw                   ; Copy 2*0x100 bytes

    ;; Load the MBR and look for the active partition
    mov si, msg_disk
    cmp dl, 0x80                ; Hard drive?
    jb error                    ; No
    mov si, mbr_part
    mov dh, 1                   ; 1 sector
    call nread

    mov si, MEM_BUF+PRT_OFF
    mov dh, 1
.1:
    cmp BYTE [si + 0x4], PRT_TYP
    jne .2
    test BYTE [si], 0x80
    jnz .3
.2:
    add si, 0x10
    inc dh
    cmp dh, PRT_NUM+1
    jb .1

    mov si, msg_part
    jmp error

.3:
    mov [MEM_ARG], dx
    mov dh, NSECT
    ;; due to MEM_BUF being 0x8e00 and stage1 being 0x200 in size
    ;; stage 2 will end up on 0x9000, or MEM_STG2
    call nread

seta20:
    cli
.1:
    ;; using cx==0 as a timeout because FreeBSD told me so,
    ;; something to do with keyboard controllers on embedded hardware
    dec cx                      ; Timeout?
    jz .3                       ; Yes
    in al, 0x64                 ; Get status
    test al, 0x2                ; Busy?
    jnz .1                      ; Yes
    mov al, 0xd1                ; Command: write
    out 0x64, al                ;  output port
.2:
    in al, 0x64                 ; Get status
    test al, 0x2                ; Busy?
    jnz .2                      ; Yes
    mov al, 0xdf                ; Enable
    out 0x60, al                ;  A20
.3:
    sti
    jmp _start-MEM_ORG+MEM_STG2 ; Jump to stage2

nread:
    mov bx, MEM_BUF
    mov ax, [si + 0x8]
    mov cx, [si + 0xa]
    call xread.1
    jnc ret
    mov si, msg_read

error:
    call puts
    mov si, msg_error
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

    ;; Reads sectors from disk
    ;; Currently only supports LBA
    ;;
    ;; dl    - drive number
    ;; stack - disk packet
read:
    cmp dl, 0x80                ; Hard drive?
    jb .1                       ; No, CHS

    mov bx, 0x55aa              ; Magic
    push dx                     ; Save
    mov ah, 0x41
    int 0x13                    ; Check BIOS extensions
    pop dx                      ; Restore
    jc .1                       ; If error, CHS
    cmp bx, 0xaa55              ; Magic?
    jne .1                      ; No, so use CHS

    test cl, 0x1                ; Packet interface?
    jz .1                       ; No, so use CHS

    mov si, bp                  ; Disk packet
    mov ah, 0x42
    int 0x13                    ; Extended read
    ret
.1:
    jmp eret                    ; CHS is not yet implemented

msg_read:   db "Read",0
msg_disk:   db "Disk",0
msg_part:   db "Partition",0
msg_error:  db " error",0xd,0xa,0

times PRT_OFF-($-$$) db 0
times 0x30 db 0
    ;; Mock-partition pointing to MBR LBA
mbr_part:
    db 0x80                     ; active
    db 0x00, 0x01, 0x00         ; start CHS
    db 0x00                     ; type
    db 0xfe, 0xff, 0xff         ; end CHS
    dd 0x0                      ; start LBA
    dd 0x1                      ; number of sectors

boot_magic: dw 0xaa55
