#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#include <kernel/arch/i386/idt.h>

void _syscall_handler(struct int_regs *r);

#endif // KERNEL_SYSCALL_H_
