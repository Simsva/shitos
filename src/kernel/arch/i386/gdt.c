#include <kernel/arch/i386/gdt.h>

extern void _gdt_flush(void);
extern void _tss_flush(void);

static void write_tss(int i);

struct gdt_entry _gdt[GDT_SIZE];
struct gdt_ptr _gdtp;
struct tss_entry _tss = { 0 };

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
    _gdtp.base = (struct gdt_entry *)&_gdt;

    _gdt_set_gate(0, 0, 0, 0, 0);

    /* access
     * bit  7  : present
     * bits 6-5: descriptor privilege level (ring)
     * bit  4  : type (0 is system segment, 1 is code or data segment)
     * bit  3  : executable
     * bit  2  : direction/conforming
     *           for data segment: 0 grows up, 1 grows down
     *           for code segment: 0 only executable from ring (bits 6-5)
     *                             1 executable from equal or lower ring
     * bit  1  : readable (for code segment) or writable (for data segment)
     * bit  0  : accessed (leave cleared)
     */

    /* flags
     * bit 7  : granularity flag, 0 means limit is in bytes
     *                            1 means it is in 4 KiB blocks (pages)
     * bit 6  : size flag, 0 for 16-bit and 1 for 32-bit
     * bit 5  : long-mode code flag, 64-bit if set (when set leave bit 6 cleared)
     * bit 4  : reserved
     * bit 3-0: part of limit, ignored by _gdt_set_gate
     */

    /* ring 0 CS and DS */
    _gdt_set_gate(1, 0, UINT32_MAX, 0x9a /* 1001 1010 */, 0xc0 /* 1100 0000 */);
    _gdt_set_gate(2, 0, UINT32_MAX, 0x92 /* 1001 0010 */, 0xc0 /* 1100 0000 */);

    /* ring 3 CS and DS */
    _gdt_set_gate(3, 0, UINT32_MAX, 0xfa /* 1111 1010 */, 0xc0 /* 1100 0000 */);
    _gdt_set_gate(4, 0, UINT32_MAX, 0xf2 /* 1111 0010 */, 0xc0 /* 1100 0000 */);

    /* task state segment */
    write_tss(5);

    _gdt_flush();
    _tss_flush();
}

static void write_tss(int i) {
    uint32_t base = (uint32_t)&_tss;
    uint32_t limit = base + sizeof _tss;

    _gdt_set_gate(i, base, limit, 0xe9 /* 1110 1001 */, 0x00);

    /* or with 3 for ring 3 */
    _tss.cs = 0x08 | 0x03;
    _tss.ds = _tss.ss = _tss.es = _tss.fs = _tss.gs = 0x10 | 0x03;
    _tss.ss0 = 0x10;
}
