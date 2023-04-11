#include <kernel/tty/tm.h>

#include <sys/utils.h>
#include <string.h>

#include "ansi_codes.h"

#define TM_WIDTH  (80)
#define TM_HEIGHT (25)
#define CURSOR    ((tm_cur_x) + (tm_cur_y)*TM_WIDTH)

#define DEFAULT_COLOR 0x07

/* control chars */
#define LEVEL_NONE 0
#define LEVEL_ESC  1
#define LEVEL_CSI  2
#define LEVEL_OSC  3

/* tm_options */
#define TM_INVERT   0x01
#define TM_CONCEAL  0x02

static uint16_t *const tm_memory = (uint16_t *)0xe0000000;
uint8_t tm_cur_x, tm_cur_y;
static uint8_t tm_cur_saved_x = 0, tm_cur_saved_y = 0;
static uint8_t tm_color = DEFAULT_COLOR, tm_esc_level = LEVEL_NONE;
static uint8_t tm_options = 0;

void tm_cursor_update(void);
uint16_t tm_parse_dec_rev(const char *, uint8_t);

void tm_cursor_update(void) {
    /* update blinking cursor */
    uint16_t tm_cursor = tm_cur_x + tm_cur_y*TM_WIDTH;
    outb(0x03d4, 14);
    outb(0x03d5, tm_cursor>>8);
    outb(0x03d4, 15);
    outb(0x03d5, tm_cursor & 0xff);
}

void tm_putc(unsigned char c) {
    switch(tm_esc_level) {
    case LEVEL_NONE: break;
    case LEVEL_ESC:  tm_handle_esc(c); return;
    case LEVEL_CSI:  tm_handle_csi(c); return;
    case LEVEL_OSC:  tm_esc_level = LEVEL_NONE; break;
    }

    /* ignore DEL */
    if(c == '\177') return;

    /* C0 control chars */
    if(c < ' ') {
        switch(c) {
        case C0_BS:
            if(tm_cur_x != 0) --tm_cur_x;
            break;

        case C0_HT:
            tm_cur_x = (tm_cur_x+1 + 7) & ~7;
            break;

        case C0_LF:
            ++tm_cur_y;
            __attribute__((fallthrough));
        case C0_CR:
            tm_cur_x = 0;
            break;

        case C0_VT: case C0_FF:
            ++tm_cur_y;
            break;

        /* ESC character (begins ESC sequence) */
        case C0_ESC:
            tm_esc_level = LEVEL_ESC;
            break;
        }
    } else {
        uint8_t real_color = tm_color;
        if(tm_options & TM_INVERT)
            real_color = (real_color>>4) | (real_color<<4);
        if(tm_options & TM_CONCEAL)
            real_color = (real_color&0xf0) | ((real_color&0xf0)>>4);

        tm_memory[CURSOR] = real_color<<8 | c;
        ++tm_cur_x;
    }

    if(tm_cur_x >= TM_WIDTH) {
        tm_cur_x = 0;
        ++tm_cur_y;
    }

    if(tm_cur_y >= TM_HEIGHT)
        tm_scroll((tm_cur_y--) - TM_HEIGHT + 1);

    tm_cursor_update();
}

/* handle single-char escape sequences */
void tm_handle_esc(unsigned char c) {
    switch(c) {
    case Fe_CSI: tm_esc_level = LEVEL_CSI; break;
    case Fe_OSC: tm_esc_level = LEVEL_OSC; break;
    }
}

char tm_csi_buf[6];
uint16_t tm_csi_args[2];
uint8_t tm_csi_buf_i = 0, tm_csi_args_i = 0;
void tm_handle_csi(unsigned char c) {
    uint8_t b;

    if(c >= '0' && c <= '9') {
        if(tm_csi_buf_i < LEN(tm_csi_buf))
            tm_csi_buf[tm_csi_buf_i++] = c;
        return;
    }
    if(tm_csi_args_i < LEN(tm_csi_args))
        tm_csi_args[tm_csi_args_i++]
            = tm_parse_dec_rev(tm_csi_buf, tm_csi_buf_i);
    tm_csi_buf_i = 0;

    /* all other valid characters count as separators, not just ';' */
    if(c >= ':' && c <= '?') return;

    tm_csi_args_i = 0;
    switch(c) {
    case CSI_CPL:
        tm_cur_x = 0;
        __attribute__((fallthrough));
    case CSI_CUU:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_y = (tm_cur_y < b)
            ? 0
            : (tm_cur_y - b);
        break;

    case CSI_CNL:
        tm_cur_x = 0;
        __attribute__((fallthrough));
    case CSI_CUD:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_y = (TM_HEIGHT-1 - tm_cur_y < b)
            ? (TM_HEIGHT-1)
            : (tm_cur_y + b);
        break;

    case CSI_CUF:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_x = (TM_WIDTH-1 - tm_cur_x < b)
            ? (TM_WIDTH-1)
            : (tm_cur_x + b);
        break;

    case CSI_CUB:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_x = (tm_cur_x < b)
            ? 0
            : (tm_cur_x - b);
        break;

    case CSI_CHA:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_x = MIN(TM_WIDTH, b) - 1;
        break;

    case CSI_HVP: /* TODO: should be handled a bit differently */
    case CSI_CUP:
        b = tm_csi_args[0] ? tm_csi_args[0] : 1;
        tm_cur_y = MIN(TM_HEIGHT, b) - 1;

        b = tm_csi_args[1] ? tm_csi_args[1] : 1;
        tm_cur_x = MIN(TM_WIDTH, b) - 1;
        break;

    case CSI_ED:
        tm_clear(tm_csi_args[0]);
        break;

    case CSI_EL:
        tm_clear_line(tm_csi_args[0]);
        break;

    case CSI_SGR:
        tm_sgr(tm_csi_args[0]);
        break;

    case CSI_SCP:
        tm_cur_saved_x = tm_cur_x;
        tm_cur_saved_y = tm_cur_y;
        break;

    case CSI_RCP:
        tm_cur_x = tm_cur_saved_x;
        tm_cur_y = tm_cur_saved_y;
        break;
    }
    tm_esc_level = LEVEL_NONE;
}

void tm_scroll(uint8_t lines) {
    memcpy((uint16_t *)tm_memory, (uint16_t *)tm_memory + lines*TM_WIDTH,
           (TM_HEIGHT-lines)*TM_WIDTH*sizeof(tm_memory[0]));
    memset((uint16_t *)tm_memory + (TM_HEIGHT-lines)*TM_WIDTH, 0,
           lines*TM_WIDTH*sizeof(tm_memory[0]));
}

void tm_clear(uint8_t mode) {
    switch(mode) {
    case 0:
        /* clear cursor down */
        memset((uint16_t *)tm_memory + tm_cur_x + tm_cur_y*TM_WIDTH, 0,
               (TM_WIDTH*TM_HEIGHT - (tm_cur_x + tm_cur_y*TM_WIDTH))
               * sizeof(tm_memory[0]));
        break;

    case 1:
        /* clear cursor up */
        memset((uint16_t *)tm_memory, 0, (tm_cur_x + tm_cur_y*TM_WIDTH + 1)
               * sizeof(tm_memory[0]));
        break;

    case 2: case 3:
        /* clear screen */
        /* 3 clears scrollback buffer too, but we do not have that */
        memset((uint16_t *)tm_memory, 0,
               TM_WIDTH*TM_HEIGHT * sizeof(tm_memory[0]));
        break;
    }
}

void tm_clear_line(uint8_t mode) {
    switch(mode) {
    case 0:
        /* clear cursor right */
        memset((uint16_t *)tm_memory + tm_cur_x + tm_cur_y*TM_WIDTH, 0,
               (TM_WIDTH - tm_cur_x) * sizeof(tm_memory[0]));
        break;

    case 1:
        /* clear cursor left */
        memset((uint16_t *)tm_memory + tm_cur_y*TM_WIDTH, 0,
               (tm_cur_x+1) * sizeof(tm_memory[0]));
        break;

    case 2:
        /* clear line */
        memset((uint16_t *)tm_memory + tm_cur_y*TM_WIDTH, 0,
               TM_WIDTH * sizeof(tm_memory[0]));
        break;
    }
}

uint8_t tm_sgr_to_vga[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
void tm_sgr(uint8_t mode) {
    if(mode >= SGR_FG_COLOR_FIRST && mode <= SGR_FG_COLOR_LAST) {
        tm_color &= 0xf0;
        tm_color |= tm_sgr_to_vga[mode - SGR_FG_COLOR_FIRST];
        return;
    }
    if(mode >= SGR_BG_COLOR_FIRST && mode <= SGR_BG_COLOR_LAST) {
        tm_color &= 0x0f;
        tm_color |= tm_sgr_to_vga[mode - SGR_BG_COLOR_FIRST] << 4;
        return;
    }
    if(mode >= SGR_FG_COLOR_BRIGHT_FIRST && mode <= SGR_FG_COLOR_BRIGHT_LAST) {
        tm_color &= 0xf0;
        tm_color |= tm_sgr_to_vga[mode - SGR_FG_COLOR_BRIGHT_FIRST] + 8;
        return;
    }
    if(mode >= SGR_BG_COLOR_BRIGHT_FIRST && mode <= SGR_BG_COLOR_BRIGHT_LAST) {
        tm_color &= 0x0f;
        tm_color |= (tm_sgr_to_vga[mode - SGR_BG_COLOR_BRIGHT_FIRST] + 8) << 4;
        return;
    }

    switch(mode) {
    case SGR_RESET:
        tm_color = DEFAULT_COLOR;
        tm_options = 0;
        break;

    case SGR_INVERT:
        tm_options |= TM_INVERT;
        break;
    case SGR_NOT_INVERT:
        tm_options &= ~TM_INVERT;
        break;

    case SGR_CONCEAL:
        tm_options |= TM_CONCEAL;
        break;
    case SGR_NOT_CONCEAL:
        tm_options &= ~TM_CONCEAL;
        break;

    case SGR_FG_COLOR_DEFAULT:
        tm_color = (tm_color&0xf0) | (DEFAULT_COLOR&0x0f);
        break;
    case SGR_BG_COLOR_DEFAULT:
        tm_color = (tm_color&0x0f) | (DEFAULT_COLOR&0xf0);
        break;
    }
}

uint16_t tm_parse_dec_rev(const char *s, uint8_t i) {
    uint16_t prod = 1, out = 0;
    char c;
    while(i) {
        c = s[--i];
        out += prod * (c-'0');
        prod *= 10;
    }
    return out;
}
