bits 16
;; org 0

section .text

global _start
_start:
    cli

    ;; segment setup
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov sp, 0x3000

    ;; save drive number for kernel
    mov byte [drive_num], dl

    sti

    mov si, str_booting
    call print

    ;; read kernel into memory at 0x10000 (segment 0x1000)
    ;; assumes that kernel binary is placed directly after the first sector
    ;; reads 20 * num_sectors sectors
.sector_start:
    mov cx, 20
    mov dl, byte [drive_num]
    mov si, disk_packet
    mov word [mem_segment], 0x1000
    mov word [sector], 1
.sector_loop:
    mov ah, 0x42
    int 13h
    jc disk_error

    add word [sector], 64
    add word [offset], 0x8000
    jnc .sector_same_segment

    add word [mem_segment], 0x1000
    mov word [offset], 0x0000
.sector_same_segment:
    loop .sector_loop

    ;; video mode: 320x200 @ 16 colors
    mov ah, 0x00
    mov al, 0x13
    int 10h

    ;; enable a20 line
    call check_a20
    cmp ax, 1
    je .a20_enabled

    ;; try BIOS/INT 15H
    mov ax, 0x2403
    int 15h
    jb .a20_bios_fail
    cmp ah, 0
    jnz .a20_bios_fail

    mov ax, 0x2402
    int 15h
    jb .a20_bios_fail
    cmp ah, 0
    jnz .a20_bios_fail

    cmp al, 1
    jz .a20_enabled

    mov ax, 0x2401
    int 15h
    jb .a20_bios_fail
    cmp ah, 0
    jnz .a20_bios_fail

    jmp .a20_enabled

.a20_bios_fail:
    ;; give up
    jmp a20_error

.a20_enabled:
    mov si, str_a20
    call print

    ;; enable PE flag (protected mode)
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;; jump to flush prefetch queue
    jmp .flush                  ; NOTE: why this?
.flush:
    lidt [idt]
    lgdt [gdtp]

    mov ax, (gdt_data_segment - gdt_start)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ;; mov ss, ax                  ; this mov does werid stuff and I do not like it
    jmp 8:entry32

bits 32
entry32:
    jmp 0x10000

bits 16

    ;; checks the status of the a20 line
    ;; preserves state
    ;;
    ;; returns: 0 in ax if the a20 line is disabled
    ;;          1 in ax if the a20 line is enabled
check_a20:
    pushf
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax                  ; ax = 0
    mov es, ax
    not ax                      ; ax = 0xffff
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax
    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xff

    cmp byte [es:di], 0xff

    pop ax
    mov byte [ds:si], al
    pop ax
    mov byte [es:di], al

    mov ax, 0
    je .exit
    mov ax, 1

.exit:
    sti

    pop si
    pop di
    pop es
    pop ds
    popf

    ret

disk_error:
    mov si, str_disk_error
    call print
    jmp seppuku

a20_error:
    mov si, str_a20_error
    call print
    jmp seppuku

print:
    xor bh, bh
    mov ah, 0x0e

    lodsb

    cmp al, 0
    je .exit

    int 0x10
    jmp print

.exit:
    ret

seppuku:
    ;; TODO: power off?
    jmp seppuku

str_booting:    db "booting...",0xa,0xd,0
str_a20:        db "a20 line is enabled",0xa,0xd,0
str_disk_error: db "disk error",0xa,0xd,0
str_a20_error:  db "failed to enable a20 line",0xa,0xd,0

drive_num:
    db 0x00

    ;; INT 13H packet
disk_packet:
    db 0x10
    db 0x00
num_sectors:
    dw 0x0040
offset:
    dw 0x0000
mem_segment:
    dw 0x0000
sector:
    dq 0x00000000

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

    ;; IDT
idt:
    dw 0
    dq 0

    ;; MBR stuff
times 510 - ($ - _start) db 0
dw 0xaa55
