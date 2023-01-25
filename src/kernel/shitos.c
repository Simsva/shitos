#include <sys/stdint.h>

volatile uint16_t *tm_memory = (uint16_t *)0xb8000;

const unsigned char alphanum[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static uint16_t tm_cursor = 0;
void putc(unsigned char c) {
    tm_memory[tm_cursor++] = 0x0f00|c;
}

void kmain(uint32_t magic) {
    char buf[16], *s = buf;
    putc('0'); putc('x');
    do
        *s++ = alphanum[magic%16];
    while(magic /= 16);
    while(--s >= buf)
        putc(*s);

    for(;;) __asm__ volatile("hlt");
}
