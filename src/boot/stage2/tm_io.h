#ifndef TM_IO_H_
#define TM_IO_H_

#include "stdint.h"

#define TM_WIDTH  (80)
#define TM_HEIGHT (25)

extern uint16_t tm_cursor;
extern uint8_t tm_line_reset;
extern uint8_t tm_color;

void tm_cursor_set(uint8_t x, uint8_t y);
void tm_cursor_update(void);

void tm_putc(unsigned char c);
void tm_puts(const char *str);
void tm_printf(const char *__restrict fmt, ...);

void tm_clear(void);
void tm_scroll(void);
void tm_init(void);

#endif // TM_IO_H_
