#ifndef I386_GDT_H_
#define I386_GDT_H_

#include <sys/stdint.h>

#define GDT_SIZE 7

struct gdt_entry {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, flags, base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t size;
    struct gdt_entry *base;
} __attribute__((packed));

extern struct gdt_entry _gdt[GDT_SIZE];
extern struct gdt_ptr _gdtp;

void _gdt_set_gate(int i, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags);
void gdt_install(void);

#endif // I386_GDT_H_
