/* FIXME: this */
#include "boot/stage2/stdint.h"

volatile uint16_t *tm_memory = (uint16_t *)0xb8000;

void kmain(uint32_t magic) {
    char buf[16], *s = buf;
    uint16_t i = 0;
    do
        *s++ = '0' + magic%10;
    while(magic /= 10);
    while(--s >= buf)
        tm_memory[i++] = 0x0f00|*s;

    for(;;) __asm__ volatile("hlt");
}
