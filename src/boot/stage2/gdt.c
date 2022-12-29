#include "gdt.h"
#include "stddef.h"
#include "stdint.h"

extern void _gdt_flush();

struct gdt_entry _gdt[GDT_SIZE];
struct gdt_ptr _gdtp;

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

void gdt_install(void) {
    _gdtp.size = (sizeof(struct gdt_entry) * GDT_SIZE)-1;
    _gdtp.base = &_gdt;

    _gdt_set_gate(0, 0, 0, 0, 0);

    /* ring 0 CS and DS */
    /* bit 7: present, 6-5: ring, 4: type, 3: executable, 2: directin/conforming
     * 1: readable/writable, 0: accessed (set 0) */
    _gdt_set_gate(1, 0, UINT32_MAX, 0x9a /* 10011010 */, 0xcf);
    _gdt_set_gate(2, 0, UINT32_MAX, 0x92 /* 10010010 */, 0xcf);

    /* NOTE: the bootloader runs completely in ring 0 */

    _gdt_flush();
}
