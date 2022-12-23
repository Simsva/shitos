#include "gdt.h"
#include "stddef.h"
#include "stdint.h"

struct gdt_t _gdt[GDT_SIZE];
struct gdt_ptr_t _gdtp;

void _gdt_set_gate(int i, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags) {
    _gdt[i].limit_low = limit & 0xffff;
    _gdt[i].flags = (limit >> 16) & 0x0f;

    _gdt[i].base_low = base & 0xffff;
    _gdt[i].base_mid = (base >> 16) & 0xff;
    _gdt[i].base_high = (base >> 24) & 0xff;

    _gdt[i].flags |= flags & 0xf0;
    _gdt[i].access = access;
}

void _gdt_install() {
    _gdtp.limit = (sizeof(struct gdt_t) * GDT_SIZE)-1;
    _gdtp.base = (uint32_t)&_gdt;

    _gdt_set_gate(0, 0, 0, 0, 0);

    /* kernel CS and DS */
    _gdt_set_gate(1, 0, UINT32_MAX, 0x9a, 0xcf);
    _gdt_set_gate(2, 0, UINT32_MAX, 0x92, 0xcf);

    /* userland CS and DS */
    _gdt_set_gate(3, 0, UINT32_MAX, 0xfa, 0xcf);
    _gdt_set_gate(4, 0, UINT32_MAX, 0xf2, 0xcf);

    /* TODO: TSS */

    _gdt_flush();
}
