bits 32

    %define KERNEL_MAP 0xc0000000
    %define PD_IND(x) ((x) >> 22)
    %define PT_START (entry_pt - KERNEL_MAP)
    %define PD_START (kernel_pd - KERNEL_MAP)

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

extern kmem_head

section .low.data

_kernel_args:
    dw 0                        ; tm_cursor
    db 0                        ; boot_options
    db 0                        ; drive_num

section .low.text

extern paging_init
global _start
_start:
    cli

    mov esp, 0x9000             ; temporary stack in unused memory

    ;; start memory allocation on the first free page after the kernel
    mov ebx, _kernel_end - KERNEL_MAP
    add ebx, 0xfff
    and ebx, 0xfffff000
    mov [kmem_head - KERNEL_MAP], ebx ; save start of memory

    ;; allocate frame bitset
    mov ecx, 0x1000000          ; last frame, TODO: calculate
    shr ecx, 12+3               ; calculate number of bytes in bitset
    add ebx, ecx
    ;; align (all page tables needed will be placed after this)
    add ebx, 0xfff
    and ebx, 0xfffff000

    ;; identity map first pt
    mov DWORD [PD_START + 4*PD_IND(0)], edx

    call paging_init

    ;; map kernel to 0xc0000000 + _kernel_lowtext_start
    ;; also identity map the first page table
;;     mov edi, ebx                ; edi = memory head = page table 1
;;     mov edx, edi
;;     or edx, 0x3
;;     mov DWORD [PD_START + 4*PD_IND(0)], edx
;;     mov DWORD [PD_START + 4*PD_IND(KERNEL_MAP)], edx

;;     ;; map whole kernel (and needed memory) in page tables
;;     mov esi, 0
;; .page_fill1:
;;     cmp esi, _kernel_lowtext_start
;;     jb .page_fill2
;;     cmp esi, ebx + 0x1000       ; map up to last used memory
;;     jge .end_page

;;     mov edx, esi
;;     or edx, 0x1                 ; present
;;     mov DWORD [edi], edx
;; .page_fill2:
;;     add edi, 4
;;     add esi, 0x1000
;;     cmp edi - ebx, 0x1000       ; outside of current pt?
;;     jb .page_fill3              ; no? continue
;;     add ebx, 0x1000             ; yes? allocate another pt
;; .page_fill3:
;;     jmp .page_fill1

;; .end_page:
    ;; map VGA text mode memory to 0xc03ff000
    ;; assuming the kernel will not be over 3-4 MiB
    mov DWORD [PT_START + 0xffc], 0xb8000 | 0x1
    ;; map last PDE to itself
    mov DWORD [PD_START + 0xffc], PD_START + 0x1 ; same as or but works in nasm

    ;; set page directory
    mov eax, PD_START
    mov cr3, eax

    ;; enable paging
    mov eax, cr0
    or eax, 0x80010000
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

global kernel_pd

align 0x1000
kernel_pd:  resb 0x1000
entry_pt:   resb 0x1000
initial_pt: resb 0x1000

stack_bottom:
    resb 0x4000                 ; 16 KiB
stack_top:
