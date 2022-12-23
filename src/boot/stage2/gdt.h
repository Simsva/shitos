#ifndef GDT_H_
#define GDT_H_

#include "stdint.h"

#define GDT_SIZE 3

struct gdt_t {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, flags, base_high;
} __attribute__((packed));

struct gdt_ptr_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern struct gdt_t _gdt[GDT_SIZE];
extern struct gdt_ptr_t _gdtp;

extern void _gdt_flush();

void _gdt_set_gate(int i, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags);
void _gdt_install();

#endif // GDT_H_
