#ifndef ISR_H_
#define ISR_H_

#include "stdint.h"

struct int_regs {
    /* data segment */
    uint32_t ds;
    /* pushed by pusha */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    /* interrupt number pushed in each isr */
    uint32_t int_no;
    /* pushed automatically on interrupt */
    uint32_t eip, cs, eflags;
};

void _isrs_install(void);

#endif // ISR_H_
