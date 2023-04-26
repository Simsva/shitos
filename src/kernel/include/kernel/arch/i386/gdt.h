#ifndef I386_GDT_H_
#define I386_GDT_H_

#include <stdint.h>

#define GDT_SIZE 7

struct gdt_entry {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, flags, base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t size;
    struct gdt_entry *base;
} __attribute__((packed));

struct tss_entry {
    uint32_t prev_tss;   /* previous TSS for hardware task switching */
    uint32_t esp0;       /* kernel stack pointer */
    uint32_t ss0;        /* kernel stack segment */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;         /* kernel ES */
    uint32_t cs;         /* kernel CS */
    uint32_t ss;         /* kernel SS */
    uint32_t ds;         /* kernel DS */
    uint32_t fs;         /* kernel FS */
    uint32_t gs;         /* kernel GS */
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

extern struct gdt_entry _gdt[GDT_SIZE];
extern struct gdt_ptr _gdtp;
extern struct tss_entry _tss;

void _gdt_set_gate(int i, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags);
void gdt_install(void);

#endif // I386_GDT_H_
