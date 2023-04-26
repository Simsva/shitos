#include <kernel/arch/i386/arch.h>
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
