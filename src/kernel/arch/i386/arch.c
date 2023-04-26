#include <kernel/arch/i386/arch.h>
#include <kernel/arch/i386/gdt.h>
#include <kernel/arch/i386/idt.h>
#include <features.h>

/**
 * Halt the processor
 */
_Noreturn void arch_fatal(void) {
    asm volatile("cli");
    for(;;) asm volatile("hlt");
}

/**
 * Pause the processor
 */
void arch_pause(void) {
    asm volatile(
        "sti\n"
        "hlt\n"
        "cli\n"
    );
}

/**
 * Set the kernel stack in the TSS
 */
void arch_set_kernel_stack(uintptr_t stack) {
    _tss.esp0 = stack;
}

/**
 * Enter user mode with the specified environment
 */
void arch_enter_user(uintptr_t entry, int argc, char *const *argv, char *const *envp, uintptr_t stack) {
    uint32_t cs = 0x18 | 0x03;
    uint32_t ds = 0x20 | 0x03;
    uint32_t eip = entry;
    uint32_t eflags = (1 << 9); /* interrupt flag */
    uint32_t esp = stack;

    asm volatile(
        "cli\n"
        "mov %0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        "pushl %0\n"  /* SS */
        "pushl %1\n"  /* ESP */
        "pushl %2\n"  /* EFLAGS */
        "pushl %3\n"  /* CS */
        "pushl %4\n"  /* EIP */
        "iret"
        :: "m"(ds), "m"(esp), "m"(eflags), "m"(cs), "m"(eip),
        /* use rdi, rsi, and rdx for arguments, as it is easier than putting
         * them on the stack */
           "D"(argc), "S"(argv), "d"(envp)
    );
}
