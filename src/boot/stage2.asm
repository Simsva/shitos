[bits 16]

    ;; Memory locations
    %define MEM_STG1 0x700      ; location of relocated stage1
    %define MEM_ARG 0x900       ; location of dx
    %define MEM_ORG 0x9000      ; origin
    %define BDA_BOOT 0x472      ; boot howto flag

section .text

global _start
_start:
    mov si, msg_stage2

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

msg_stage2: db "stage2 woohoo",0
msg_error:  db " error",0xd,0xa,0
