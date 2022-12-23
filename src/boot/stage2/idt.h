#ifndef IDT_H_
#define IDT_H_

#include "stdint.h"
#include "isr.h"

#define IDT_SIZE 0x100

struct idt_entry {
    uint16_t off_low, sel;
    uint8_t reserved, flags;
    uint16_t off_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t size;
    struct idt_entry *base;
} __attribute__((packed));

extern struct idt_entry _idt[IDT_SIZE];
extern struct idt_ptr _idtp;

extern void _idt_load();

void _idt_set_gate(uint8_t i, uint32_t offset,
                   uint16_t selector, uint8_t flags);
void _idt_install(void);

#endif // IDT_H_
