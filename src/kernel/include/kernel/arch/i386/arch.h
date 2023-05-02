#ifndef I386_ARCH_H_
#define I386_ARCH_H_

#include <kernel/arch/i386/idt.h>
#include <stdint.h>

void arch_fatal(void);
void arch_pause(void);

void arch_set_kernel_stack(uintptr_t stack);
void arch_enter_user(uintptr_t entry, int argc, char *const *argv, int envc, char *const *envp, uintptr_t stack);

void arch_syscall_ret(struct int_regs *r, long ret);
long arch_syscall_num(struct int_regs *r);
long arch_syscall_arg0(struct int_regs *r);
long arch_syscall_arg1(struct int_regs *r);
long arch_syscall_arg2(struct int_regs *r);
long arch_syscall_arg3(struct int_regs *r);
long arch_syscall_arg4(struct int_regs *r);
long arch_syscall_arg5(struct int_regs *r);

#endif // I386_ARCH_H_
