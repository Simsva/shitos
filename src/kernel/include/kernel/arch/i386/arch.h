#ifndef I386_ARCH_H_
#define I386_ARCH_H_

#include <stdint.h>

void arch_fatal(void);
void arch_pause(void);

void arch_set_kernel_stack(uintptr_t stack);
void arch_enter_user(uintptr_t entry, int argc, char *const *argv, char *const *envp, uintptr_t stack);

#endif // I386_ARCH_H_
