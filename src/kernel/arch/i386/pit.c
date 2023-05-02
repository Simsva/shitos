#include <kernel/arch/i386/ports.h>
#include <kernel/arch/i386/irq.h>

#define PIT_A       0x40
#define PIT_B       0x41
#define PIT_C       0x42
#define PIT_CONTROL 0x43

#define PIT_SCALE 1193180
#define PIT_SET 0x34

#define PIT_IRQ 0

uintmax_t pit_ticks = 0;

static void pit_set_phase(long hz) {
    long div = PIT_SCALE / hz;
    outportb(PIT_CONTROL, PIT_SET);
    outportb(PIT_A, div & 0xff);
    outportb(PIT_A, (div >> 8) & 0xff);
}

static void pit_reset_ticks(void) {
    pit_ticks = 0;
}

void pit_handler(__unused struct int_regs *r) {
    pit_ticks++;
}

void pit_init(void) {
    irq_handler_install(PIT_IRQ, pit_handler);
    pit_set_phase(1000);
    pit_reset_ticks();
}
