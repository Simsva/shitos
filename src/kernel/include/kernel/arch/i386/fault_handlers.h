#ifndef I386_FAULT_HANDLERS_H_
#define I386_FAULT_HANDLERS_H_

#include <kernel/arch/i386/idt.h>

extern void _page_fault(struct int_regs *r);

#endif // I386_FAULT_HANDLERS_H_
