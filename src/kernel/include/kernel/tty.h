#ifndef KERNEL_TTY_H_
#define KERNEL_TTY_H_

#include <stdint.h>

/* text modes */
#define ANSI_MODE_BOLD          0x0001
#define ANSI_MODE_FAINT         0x0002
#define ANSI_MODE_ITALIC        0x0004
#define ANSI_MODE_UNDERLINE     0x0008
#define ANSI_MODE_SLOW_BLINK    0x0010
#define ANSI_MODE_FAST_BLINK    0x0020
#define ANSI_MODE_INVERT        0x0040
#define ANSI_MODE_CONCEAL       0x0080
#define ANSI_MODE_STRIKE        0x0100
#define ANSI_MODE_DBL_UNDERLINE 0x0200
#define ANSI_MODE_FRAKTUR       0x0400
#define ANSI_MODE_FRAMED        0x0800
#define ANSI_MODE_ENCIRCLED     0x1000
#define ANSI_MODE_OVERLINE      0x2000
#define ANSI_MODE_SUPERSCRIPT   0x4000
#define ANSI_MODE_SUBSCRIPT     0x8000

/* ANSI_OUT_ESC return values */
#define ANSI_ESC_CSI_ED         0
#define ANSI_ESC_CSI_EL         1
#define ANSI_ESC_Fe_SS2         2
#define ANSI_ESC_Fe_SS3         3
#define ANSI_ESC_Fe_DCS         4
#define ANSI_ESC_Fe_CSI         5
#define ANSI_ESC_Fe_ST          6
#define ANSI_ESC_Fe_OSC         7
#define ANSI_ESC_Fe_SOS         8
#define ANSI_ESC_Fe_PM          9
#define ANSI_ESC_Fe_APC         10

enum ansi_out {
ANSI_OUT_CHAR,      /* normal character */
ANSI_OUT_ESC,       /* handle escape sequence */
ANSI_OUT_SCROLL,    /* scroll oc lines */
};

typedef struct ansi_ctx ansi_ctx_t;
typedef void (*ansi_callback_t)(ansi_ctx_t *, enum ansi_out, uint32_t);
struct ansi_ctx {
    ansi_callback_t callback;        /* called on ansi_out events */
    uint16_t args[6];                /* arguments to last escape sequence */
    uint8_t args_sz;                 /* argument count */
    char arg_buf[6];                 /* internal argument buffer */
    uint8_t arg_buf_i;               /* internal argument buffer index */
    uint16_t text_mode;              /* ANSI_MODE_* */
    uint32_t color_fg, color_bg;     /* text color */
    uint32_t color_def_fg,
             color_def_bg;           /* default text color */
    uint8_t color_fmt;               /* 0 for 4-bit, 2 for 24-bit */
    uint8_t esc_level;               /* current escape mode */
    uint16_t width, height;          /* terminal dimensions */
    uint16_t cur_x, cur_y;           /* cursor position (zero-indexed) */
    uint16_t scur_x, scur_y;         /* saved cursor position */
};

void tm_term_install(void);
void fb_term_install(const char *font_path);

void ansi_init(ansi_ctx_t *ctx, uint16_t w, uint16_t h, uint32_t color_fmt,
               uint32_t color_def_fg, uint32_t color_def_bg,
               ansi_callback_t callback);
void ansi_handle(ansi_ctx_t *ctx, uint32_t c);

#endif // KERNEL_TTY_H_
