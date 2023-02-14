#ifndef FAULT_HANDLERS_H_
#define FAULT_HANDLERS_H_

#include "idt.h"

extern void _page_fault(struct int_regs *r);

#endif // FAULT_HANDLERS_H_
