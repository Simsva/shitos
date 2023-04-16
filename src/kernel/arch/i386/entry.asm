bits 32

    %define KERNEL_MAP 0xc0000000
    %define PD_IND(x) ((x) >> 22)
    %define PT_START (entry_pt - KERNEL_MAP)
    %define PD_START (kernel_pd - KERNEL_MAP)

extern kernel_pd

extern gdt_install
extern idt_install
extern irq_install
extern isrs_install

extern vmem_init

extern _kernel_lowtext_start
extern _kernel_text_start
extern _kernel_rodata_start
extern _kernel_data_start
extern _kernel_bss_start
extern _kernel_end

extern frames
extern frame_count
extern kmem_head

section .low.data

_kernel_args:
    ;; XXX: this is manually changed to reflect the size of kernel args
times 20 db 0

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

    ;; allocate frame bitset
    mov DWORD [frames - KERNEL_MAP], ebx
    mov ecx, 0x1000000          ; last frame, TODO: calculate
    shr ecx, 12                 ; calculate number of frames
    mov DWORD [frame_count - KERNEL_MAP], ecx
    shr ecx, 3                  ; calculate number of bytes in bitset
    add ebx, ecx
    ;; align (all page tables needed will be placed after this)
    add ebx, 0xfff + 1          ; add one additional byte to the bitset
    and ebx, 0xfffff000
    mov DWORD [kmem_head - KERNEL_MAP], ebx ; save start of memory

    call paging_init

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

    call vmem_init

    ;; initialize the FPU
    mov eax, cr0
    or eax, 1<<1 | 1<<4
    and eax, ~(1<<2 | 1<<3)
    mov cr0, eax
    fninit

    sti

    ;; call kmain
    push _kernel_args + KERNEL_MAP
    push DWORD 0
    jmp kmain

section .bss

stack_bottom:
    resb 0x4000                 ; 16 KiB
stack_top:
