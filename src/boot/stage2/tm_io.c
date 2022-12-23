#include "tm_io.h"

#include "io.h"
#include "string.h"

uint16_t tm_cursor = 0;
uint8_t tm_color = 0x0f;

void tm_cursor_set(uint8_t x, uint8_t y) {
    tm_cursor = x + TM_WIDTH*y;
}

void tm_cursor_update(void) {
    /* update blinking cursor */
    outb(0x03d4, 14);
    outb(0x03d5, tm_cursor>>8);
    outb(0x03d4, 15);
    outb(0x03d5, tm_cursor & 0xff);
}

void tm_putc(unsigned char c) {
    /* TODO: handle control chars */
    if(c < ' ') return;

    tm_memory[tm_cursor++] = c | (tm_color<<8);
    tm_cursor %= TM_WIDTH*TM_HEIGHT;
    tm_cursor_update();
}

void tm_puts(const char *str) {
    for(const char *c = str; *c; c++)
        tm_putc(*c);
}

void tm_clear(void) {
    memset(tm_memory, 0, sizeof(tm_memory[0]) * TM_WIDTH*TM_HEIGHT);
    tm_cursor = 0;
    tm_cursor_update();
}

void tm_init(void) {
    tm_color = 0x0f;
    tm_clear();
}
