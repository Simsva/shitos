bits 32

    %define KERN_ESP 0x20000

section .text

    ;; call kmain with a known environment
global call_kmain
call_kmain:
    add esp, 4                  ; ignore return address
    pop eax                     ; *kmain
    pop ecx                     ; size of kernel args

    mov esi, esp
    mov edi, KERN_ESP
    sub edi, ecx
    mov edx, edi                ; save new esp
    rep movsb                   ; copy kernel args

    mov esp, edx
    push DWORD 0                ; empty return addr
    mov ebp, esp
    jmp eax                     ; call kmain
