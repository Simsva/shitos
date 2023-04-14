#include <kernel/arch/i386/error.h>
#include <features.h>

/**
 * Halt the processor
 */
_Noreturn void arch_fatal(void) {
    asm volatile("cli");
    for(;;) asm volatile("hlt");
}
