#include "tm_io.h"

#include <sys/string.h>
#include <sys/stdarg.h>
#include <sys/utils.h>

static uint16_t *tm_memory = (uint16_t *)0xb8000;
uint16_t tm_cursor = 0;
uint8_t tm_line_reset = 0;
uint8_t tm_color = 0x0f;
static unsigned char alphanum[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

inline uint8_t chtoi(char c) {
    return c >= 'a'
         ? c - 'a' + 0xa
         : c - '0';
}

void tm_cursor_set(uint8_t x, uint8_t y) {
    tm_cursor = x + TM_WIDTH*y;
}

void tm_cursor_update(void) {
    /* update blinking cursor */
    /* outb(0x03d4, 14); */
    /* outb(0x03d5, tm_cursor>>8); */
    /* outb(0x03d4, 15); */
    /* outb(0x03d5, tm_cursor & 0xff); */
}

void tm_putc(unsigned char c) {
    if(c < ' ') {
        /* control chars */
        uint8_t x = tm_cursor % TM_WIDTH,
                y = tm_cursor / TM_WIDTH;

        switch(c) {
        case '\b':
            if(x != 0) --x;
            break;

        case '\t':
            /* & ~7 to truncate to a multiple of 8 */
            x = (x + 8) & ~7;
            break;

        case '\n':
            ++y;
        case '\r':
            x = tm_line_reset;
            break;
        }

        tm_cursor = x + y*TM_WIDTH;
    } else {
        tm_memory[tm_cursor++] = c | (tm_color<<8);
    }

    if(tm_cursor >= TM_WIDTH*TM_HEIGHT) {
        tm_scroll();
        tm_cursor -= TM_WIDTH;
    }
    /* tm_cursor %= TM_WIDTH*TM_HEIGHT; */
    tm_cursor_update();
}

void tm_puts(const char *str) {
    unsigned char c;
    while((c = *str++))
        tm_putc(c);
    tm_putc('\n');
}

void tm_printf(const char *__restrict fmt, ...) {
    va_list ap;
    static unsigned char buf[10];
    unsigned char c, *s;
    unsigned int u;

    va_start(ap, fmt);
    while((c = *fmt++)) {
        if(c == '%') {
            c = *fmt++;
            switch(c) {
            case 'c':
                tm_putc(va_arg(ap, int));
                continue;

            case 's':
                for(s = va_arg(ap, unsigned char *); *s; s++)
                    tm_putc(*s);
                continue;

            case 'u':
                u = va_arg(ap, unsigned int);
                s = buf;
                do
                    *s++ = '0' + u%10;
                while(u /= 10);
                while(--s >= buf)
                    tm_putc(*s);
                continue;

            case 'x':
                u = va_arg(ap, unsigned int);
                s = buf;
                do
                    *s++ = alphanum[u%16];
                while(u /= 16);
                while(s-- >= buf)
                    tm_putc(*s);
                continue;

            case 'o':
                u = va_arg(ap, unsigned int);
                s = buf;
                do
                    *s++ = '0' + u%8;
                while(u /= 8);
                while(--s >= buf)
                    tm_putc(*s);
                continue;

            case 'h':
                tm_color = va_arg(ap, unsigned int);
                continue;

            case 'H':
                tm_color = chtoi(*fmt++)<<4;
                tm_color |= chtoi(*fmt++);
                continue;

            case '%':
                tm_putc('%');
                continue;
            }
        }

        tm_putc(c);
    }

    va_end(ap);
}

void tm_clear(void) {
    memset(tm_memory, 0, sizeof(tm_memory[0]) * TM_WIDTH*TM_HEIGHT);
    tm_cursor = 0;
    tm_cursor_update();
}

void tm_scroll(void) {
    memcpy(tm_memory, tm_memory + TM_WIDTH,
           (TM_HEIGHT-1)*TM_WIDTH*sizeof(tm_memory[0]));
    memset(tm_memory+(TM_HEIGHT-1)*TM_WIDTH, 0,
           TM_WIDTH*sizeof(tm_memory[0]));
}

void tm_init(void) {
    tm_color = 0x0f;
    tm_clear();
}
