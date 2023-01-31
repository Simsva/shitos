bits 32

    ;; memory locations
    %define MEM_ESPR  0x5e00    ; real mode stack

    ;; _v86 offsets
    %define V86_CTL    0x0      ; Control flags
    %define V86_ADDR   0x4      ; Int number/address
    %define V86_ES     0x8      ; V86 ES
    %define V86_DS     0xc      ; V86 DS
    %define V86_FS     0x10     ; V86 FS
    %define V86_GS     0x14     ; V86 GS
    %define V86_EAX    0x18     ; V86 EAX
    %define V86_ECX    0x1c     ; V86 ECX
    %define V86_EDX    0x20     ; V86 EDX
    %define V86_EBX    0x24     ; V86 EBX
    %define V86_EFL    0x28     ; V86 eflags
    %define V86_EBP    0x2c     ; V86 EBP
    %define V86_ESI    0x30     ; V86 ESI
    %define V86_EDI    0x34     ; V86 EDI

    ;; globals
    %define INT_V86    0x31     ; v86 interrupt number
    %define SZ_V86     0x38     ; size of _v86 structure

    ;; v86 control flags
    %define V86F_ADDR  0x10000  ; segment:offset address
    %define V86F_FLAGS 0x20000  ; return flags

    ;; GDT selectors
    %define SEL_KCODE  0x8      ; kernel code
    %define SEL_KDATA  0x10     ; kernel data
    %define SEL_RCODE  0x18     ; real mode code
    %define SEL_RDATA  0x20     ; real mode data

    ;; eflags
    %define EFL_DEF    0x00000002 ; default eflags (reserved)
    %define EFL_TF     0x00000100 ; trap flag
    %define EFL_IF     0x00000200 ; interrupt enable flag
    %define EFL_DF     0x00000400 ; string operation direction
    %define EFL_NT     0x00004000 ; nested task flag
    %define EFL_VM     0x00020000 ; virtual 8086 mode flag
    %define EFL_AC     0x00040000 ; alignment check flag

global _v86int
global _v86
global _isr_v86

section .text

_v86int:
    pop DWORD [_v86ret]         ; save return addr
    push _v86                   ; push _v86 ptr
    call _v86_swap              ; swap to v86 registers
    int INT_V86                 ; call v86 interrupt
    call _v86_swap              ; restore user registers
    add esp, 0x4                ; discard _v86 ptr
    push DWORD [_v86ret]        ; restore return addr
    ret

_v86_swap:
    xchg ebp, [esp + 0x4]       ; swap _v86 ptr and ebp

    ;; swap general registers
    xchg eax, [ebp + V86_EAX]
    xchg ecx, [ebp + V86_ECX]
    xchg edx, [ebp + V86_EDX]
    xchg ebx, [ebp + V86_EBX]

    ;; swap eflags
    push eax                    ; save
    pushf
    pop eax
    xchg eax, [ebp + V86_EFL]
    push eax
    popf

    ;; swap ebp
    mov eax, [esp + 0x8]        ; load ebp
    xchg eax, [ebp + V86_EBP]   ; swap ebp
    mov [esp + 0x8], eax        ; save ebp
    pop eax                     ; restore eax

    ;; swap esi + edi
    xchg esi, [ebp + V86_ESI]
    xchg edi, [ebp + V86_EDI]

    xchg ebp, [esp + 0x4]       ; swap _v86 ptr and ebp
    ret

    ;; Invoke real mode interrupt/function call from protected mode.
    ;;
    ;; We place a trampoline on the user stack that will return to rret_tramp
    ;; which will reenter protected mode and then finally return to the user
    ;; client.
    ;;
    ;; Kernel/boot frame esi points to:     Real mode stack frame at MEM_ESPR:
    ;; -0x00 _v86 ptr                       -0x04 kernel esp
    ;; -0x04 eflags                         -0x08 _v86 pointer
    ;; -0x08 cs                             -0x0c flags (only used if interrupt)
    ;; -0x0c eip                            -0x10 real mode cs:ip return trampoline
    ;; -0x10 eax                            -0x12 real mode flags
    ;; -0x14 ecx                            -0x16 real mode cs:ip of target
    ;; -0x18 edx
    ;; -0x1c ebx
    ;; -0x20 esp
    ;; -0x24 ebp
    ;; -0x28 esi
    ;; -0x2c edi
    ;; -0x30 gs
    ;; -0x34 fs
    ;; -0x38 ds
    ;; -0x3c es
_isr_v86:
    ;; cli                         ; disable interrupts
    cld                         ; string ops inc

    pusha                       ; save gp regs
    push gs                     ; save
    push fs                     ;  seg
    push ds                     ;  regs
    push es

    mov eax, SEL_KDATA          ; set up
    mov ds, eax                 ;  to address
    mov es, eax                 ;  data

    lea esi, [esp + 0x3c]       ; base of frame
    mov [MEM_ESPR - 0x4], esp   ; save kernel esp

    mov edx, [esi - 0x0]        ; _v86 ptr
    mov [MEM_ESPR - 0x8], edx   ; save _v86 ptr

    mov eax, [edx + V86_ADDR]   ; get int no/address
    mov edx, [edx + V86_CTL]    ; get control flags
    mov ebx, [esi - 0x4]        ; save eflags in ebx
    mov [MEM_ESPR - 0xc], ebx   ; copy eflags to real mode trampoline

    test edx, V86F_ADDR         ; segment:offset?
    jnz .1                      ; yes

    and ebx, ~(EFL_IF|EFL_TF|EFL_AC) ; disable interrupts, traps, and alignment
                                ;  checking for interrupt handler

    ;; look up real mode IDT entry
    shl eax, 2                  ; scale
    mov eax, [eax]              ; load int vector
;;     jmp .2                      ; skip CALLF test

;; .1:
;;     test edx, V86F_CALLF        ; far call?

    ;; eax now holds the segment:offset of the function
    ;; ebx now holds the eflags to pass to real mode
    ;; edx now holds the v86 control flags
.1:
.2:
    mov [MEM_ESPR - 0x12], bx   ; pass flags to real mode

    mov ecx, [MEM_ESPR - 0x8]   ; get _v86 ptr
    lea edi, [esi - 0x3c]       ; edi => kernel stack seg regs
    push esi                    ; save
    lea esi, [ecx + V86_ES]     ; esi => _v86 seg regs
    mov ecx, 4                  ; copy seg regs from
    rep movsd                   ;  _v86 to kernel stack
    pop esi                     ; restore

    mov ebx, rret_tramp         ; set return trampoline
    mov [MEM_ESPR - 0x10], ebx  ;  cs:ip
    mov [MEM_ESPR - 0x16], eax  ; real mode target cs:ip

    jmp SEL_RCODE:.3            ; change to 16-bit segment
bits 16
.ivtp:  dw 0x400-0x0-0, 0x0, 0x0
.3:
    mov eax, cr0                ; leave
    dec al                      ;  protected
    mov cr0, eax                ;  mode

    jmp 0:.4
.4:
    xor ax, ax                  ; reset
    mov ds, ax                  ;  data
    mov ss, ax                  ;  segments
    lidt [.ivtp]                ; set IDT
    ;; NOTE: length modifier because I do not know how to do `popl %gs` in NASM
    db 0x66
    pop es                      ; restore
    db 0x66
    pop ds                      ;  seg
    db 0x66
    pop fs                      ;  regs
    db 0x66
    pop gs
    popad                       ; restore gp regs
    mov sp, MEM_ESPR - 0x16     ; switch to real mode stack
    iret                        ; call target routine

    ;; We set up a stack frame that looks like this on the real mode stack.
    ;; Note that far calls do not pop the flags, which we ignore by
    ;; repositioning sp to be just above the _v86 pointer. The stack is
    ;; relative to MEM_ESPR.
    ;;
    ;; -0x04 kernel esp
    ;; -0x08 _v86 pointer
    ;; -0x0c eax
    ;; -0x10 ecx
    ;; -0x14 edx
    ;; -0x18 ebx
    ;; -0x1c esp
    ;; -0x20 ebp
    ;; -0x24 esi
    ;; -0x28 edi
    ;; -0x2c gs
    ;; -0x30 fs
    ;; -0x34 ds
    ;; -0x38 es
    ;; -0x3c eflags
rret_tramp:
    mov sp, MEM_ESPR - 0x8      ; reset stack pointer
    pushad                      ; save gp regs
    ;; NOTE: length modifier because I do not know how to do `pushl %gs` in NASM
    db 0x66
    push gs                     ; save
    db 0x66
    push fs                     ;  seg
    db 0x66
    push ds                     ;  regs
    db 0x66
    push es
    pushfd                      ; save eflags
    push DWORD EFL_DEF|EFL_DF   ; use clean eflags with
    popfd                       ;  string ops dec

    xor ax, ax                  ; reset seg
    mov ds, ax                  ;  regs
    mov es, ax                  ;  (ss is already 0)

extern _idtp
    lidt [_idtp]                ; set IDT
extern _gdtp
    lgdt [_gdtp]                ; set GDT

    mov eax, cr0                ; switch to
    inc ax                      ;  protected
    mov cr0, eax                ;  mode

    jmp SEL_KCODE:.1            ; to 32-bit code
bits 32
.1:
    xor ecx, ecx                ; zero
    mov cl, SEL_KDATA           ; setup
    mov ds, cx                  ;  32-bit
    mov es, cx                  ;  seg
    mov ss, cx                  ;  regs

    mov esp, [MEM_ESPR - 0x4]   ; switch to kernel stack
    lea esi, [esp + 0x3c]       ; base of frame

    ;; NOTE: clear TSS busy + ltr if using TSS

    ;; Copy registers off of the real mode stack onto the kernel stack to get
    ;; their updated values. Also, initialize the segment registers on the
    ;; kernel stack.
    ;;
    ;; NOTE: the esp in the kernel stack after this is garbage, but popa ignores
    ;; it, so we do not have to fix it up.
    lea edi, [esi - 0x10]       ; kernel stack gp regs
    push esi                    ; save
    mov esi, MEM_ESPR - 0xc     ; real mode stack gp regs
    mov ecx, 8                  ; copy gp regs from real mode
    rep movsd                   ;  stack to kernel stack

    mov eax, SEL_KDATA          ; selector for data seg regs
    mov ecx, 4                  ; initialize ds,
    rep stosd                   ;  es, fs, and gs

    ;; Copy saved regs on the real mode stack over to the _v86 struct. Also,
    ;; conditionally update the saved eflags on the kernel stack based on the
    ;; flags from the user.
    mov ecx, [MEM_ESPR - 0x8]   ; get _v86 ptr
    lea edi, [ecx + V86_GS]     ; edi => _v86 seg regs
    mov esi, MEM_ESPR - 0x2c    ; esi => real mode seg regs
    xchg ecx, edx               ; save _v86 ptr
    mov ecx, 4                  ; copy seg regs from real mode
    rep movsd                   ;  stack to _v86
    pop esi                     ; restore

    mov edx, [edx + V86_CTL]    ; read v86 control flags
    test edx, V86F_FLAGS        ; user wants flags?
    jz .2                       ; no

    mov eax, [MEM_ESPR - 0x3c]  ; read real mode flags
    and eax, ~(EFL_TF|EFL_NT)   ; clear unsafe flags
    mov [esi - 0x4], ax         ; update flags (low 16)

.2:
    pop es                      ; restore
    pop ds                      ;  seg
    pop fs                      ;  regs
    pop gs
    popa                        ; restore gp regs

    iret

section .data
_v86:       times SZ_V86 db 0x0
_v86ret:    dd 0x0
