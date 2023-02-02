bits 32

    %define KERN_ARGS 0x20000   ; always at the beginning of the binary

section .text

    ;; call kernel with kernel args
global call_kernel
call_kernel:
    add esp, 4                  ; ignore return address
    pop eax                     ; *kmain
    pop ecx                     ; size of kernel args

    mov esi, esp
    mov edi, KERN_ARGS
    rep movsb                   ; copy kernel args

    jmp eax                     ; call kmain
