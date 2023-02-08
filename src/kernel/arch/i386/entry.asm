bits 32

    %define KERNEL_MAP 0xc0000000
    %define PD_IND(x) ((x) >> 22)
    %define PT_START (entry_pt - KERNEL_MAP)
    %define PD_START (entry_pd - KERNEL_MAP)

extern gdt_install
extern idt_install
extern irq_install
extern isrs_install

extern _kernel_lowtext_start
extern _kernel_text_start
extern _kernel_rodata_start
extern _kernel_data_start
extern _kernel_bss_start
extern _kernel_end

section .low.data

_kernel_args:
    dw 0                        ; tm_cursor
    db 0                        ; boot_options
    db 0                        ; drive_num

section .low.text

global _start
_start:
    cli

    ;; map kernel to 0xc0000000 + _kernel_lowtext_start
    ;; also identity map it
    mov edi, PT_START
    mov edx, edi
    or edx, 0x3
    mov DWORD [PD_START + 4*PD_IND(0)], edx
    mov DWORD [PD_START + 4*PD_IND(KERNEL_MAP)], edx

    ;; test page
    mov edx, PT_START + 0x1000
    or edx, 0x3
    mov DWORD [PD_START + 4], edx

    ;; long boilerplateish paging things for ELF sections
    mov esi, 0
.page_lowtext1:
    cmp esi, _kernel_lowtext_start
    jb .page_lowtext2
    cmp esi, _kernel_text_start - KERNEL_MAP
    jge .page_text

    mov edx, esi
    or edx, 0x1                 ; present
    mov DWORD [edi], edx
.page_lowtext2:
    add edi, 4
    add esi, 0x1000
    jmp .page_lowtext1

.page_text:
    cmp esi, _kernel_rodata_start - KERNEL_MAP
    jge .page_rodata

    mov edx, esi
    or edx, 0x1                 ; present
    mov DWORD [edi], edx

    add edi, 4
    add esi, 0x1000
    jmp .page_text

.page_rodata:
    cmp esi, _kernel_data_start - KERNEL_MAP
    jge .page_data

    mov edx, esi
    or edx, 0x1                 ; present
    mov DWORD [edi], edx

    add edi, 4
    add esi, 0x1000
    jmp .page_rodata

.page_data:
    cmp esi, _kernel_bss_start - KERNEL_MAP
    jge .page_bss

    mov edx, esi
    or edx, 0x3                 ; rw + present
    mov DWORD [edi], edx

    add edi, 4
    add esi, 0x1000
    jmp .page_data

.page_bss:
    cmp esi, _kernel_end - KERNEL_MAP
    jge .end_page

    mov edx, esi,
    or edx, 0x3                 ; rw + present
    mov DWORD [edi], edx,

    add edi, 4
    add esi, 0x1000
    jmp .page_bss

.end_page:
    ;; map VGA text mode memory to 0xc03ff000
    ;; assuming the kernel will not be over 3-4 MiB
    mov DWORD [PT_START + 0xffc], 0xb8000 | 0x3
    ;; map last PDE to itself
    mov DWORD [PD_START + 0xffc], PD_START + 0x3 ; same as or but works in nasm

    ;; set page directory
    mov eax, PD_START
    mov cr3, eax

    ;; enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lea eax, _highstart
    jmp eax

section .text

extern kmain

_highstart:
    ;; remove kernel identity mapping
    mov DWORD [PD_START + 4*PD_IND(0)], 0
    mov eax, cr3
    mov cr3, eax

    mov esp, stack_top

    ;; i386 initialization
    call gdt_install
    call idt_install
    call irq_install
    call isrs_install

    sti

    ;; call kmain
    push _kernel_args + KERNEL_MAP
    push DWORD 0
    jmp kmain

section .bss

align 0x1000
entry_pd:   resb 0x1000
entry_pt:   resb 0x1000
test_pt:    resb 0x1000

stack_bottom:
    resb 0x4000                 ; 16 KiB
stack_top:
