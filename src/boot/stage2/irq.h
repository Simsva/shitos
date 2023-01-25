#ifndef IRQ_H_
#define IRQ_H_

#include <sys/stdint.h>
#include "idt.h"

typedef void (*irq_handler_t)(struct int_regs *);

void pic_remap(uint8_t off_pic1, uint8_t off_pic2);

void irq_handler_install(uint8_t irq, irq_handler_t handler);
void irq_handler_uninstall(uint8_t irq);
void irq_install(void);

#endif // IRQ_H_
