#include "irq.h"

#include <sys/utils.h>
#include <stddef.h>

/* PIC1 = master PIC and PIC2 = slave PIC */
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC2_CMD        0xa0
#define PIC2_DATA       0xa1
#define PIC_EOI         0x20        /* end-of-interrupt command code */

#define ICW1_ICW4       0x01        /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02        /* single (cascade) mode */
#define ICW1_INTERVAL4  0x04        /* call address interval 4 (8) */
#define ICW1_LEVEL      0x08        /* level triggered (edge) mode */
#define ICW1_INIT       0x10        /* initialization - required! */

#define ICW4_8086       0x01        /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02        /* auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08        /* buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C        /* buffered mode/master */
#define ICW4_SFNM       0x10        /* special fully nested (not) */

#define IRQ0_OFFSET     0x20        /* offset of master PIC IRQs */
#define IRQ8_OFFSET     0x28        /* offset of slave PIC IRQs */

extern void _irq0(void);
extern void _irq1(void);
extern void _irq2(void);
extern void _irq3(void);
extern void _irq4(void);
extern void _irq5(void);
extern void _irq6(void);
extern void _irq7(void);
extern void _irq8(void);
extern void _irq9(void);
extern void _irqa(void);
extern void _irqb(void);
extern void _irqc(void);
extern void _irqd(void);
extern void _irqe(void);
extern void _irqf(void);

static irq_handler_t _irq_handlers[16] = { NULL };

void irq_handler_install(uint8_t irq, irq_handler_t handler) {
    _irq_handlers[irq] = handler;
}

void irq_handler_uninstall(uint8_t irq) {
    _irq_handlers[irq] = NULL;
}

void pic_remap(uint8_t off_pic1, uint8_t off_pic2) {
    uint8_t a1, a2;

    /* save masks */
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    /* io_wait() gives the PIC time to react to commands on older machines */

    /* ICW1: start the initialization sequence (in cascade mode) */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: PIC vector offset */
    outb(PIC1_DATA, off_pic1);
    io_wait();
    outb(PIC2_DATA, off_pic2);
    io_wait();

    /* ICW3: tell master PIC that there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC1_DATA, 4);
    io_wait();
    /* ICW3: tell slave PIC its cascade identity (0000 0010) */
    outb(PIC2_DATA, 2);
    io_wait();

    /* ICW4 */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* restore saved masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void irq_install(void) {
    /* Remap IRQ0--15 to IDT entries 0x20--0x2f.
     * By default IRQ0--8 are mapped to entries 0x8--0xf, which overlap with
     * faults in protected mode.
     * For example IRQ0 maps to IDT entry 8 which is a double fault. */
    pic_remap(IRQ0_OFFSET, IRQ8_OFFSET);

    /* flag bit 7: present, 6-5: ring, 4: zero, 3-0: type */
    /* type 1110 (0xe) = interrupt gate */
    _idt_set_gate(IRQ0_OFFSET+0, (uint32_t)_irq0, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+1, (uint32_t)_irq1, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+2, (uint32_t)_irq2, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+3, (uint32_t)_irq3, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+4, (uint32_t)_irq4, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+5, (uint32_t)_irq5, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+6, (uint32_t)_irq6, 0x08, 0x8e);
    _idt_set_gate(IRQ0_OFFSET+7, (uint32_t)_irq7, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+0, (uint32_t)_irq8, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+1, (uint32_t)_irq9, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+2, (uint32_t)_irqa, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+3, (uint32_t)_irqb, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+4, (uint32_t)_irqc, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+5, (uint32_t)_irqd, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+6, (uint32_t)_irqe, 0x08, 0x8e);
    _idt_set_gate(IRQ8_OFFSET+7, (uint32_t)_irqf, 0x08, 0x8e);
}

void _irq_handler(struct int_regs r) {
    irq_handler_t handler;

    /* IRQs use int_no to store the IRQ number */
    /* To get the actual interrupt number, look at IRQ0_OFFSET and IRQ8_OFFSET */
    handler = _irq_handlers[r.int_no];

    if(handler && r.int_no != 0)
        handler(&r);

    /* if slave IRQ (8-15), send EOI to the slave controller */
    if(r.int_no > 7)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);

    /* handle IRQ0 (timer) after EOI in case of task switching */
    if(handler && r.int_no == 0)
        handler(&r);
}
