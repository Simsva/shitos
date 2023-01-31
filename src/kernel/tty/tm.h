#ifndef TTY_TM_H_
#define TTY_TM_H_

#include <sys/stdint.h>

extern uint8_t tm_cur_x, tm_cur_y;

void tm_putc(unsigned char c);
void tm_handle_esc(unsigned char c);
void tm_handle_csi(unsigned char c);

void tm_scroll(uint8_t lines);
void tm_clear(uint8_t mode);
void tm_clear_line(uint8_t mode);
void tm_sgr(uint8_t mode);

#endif // TTY_TM_H_
