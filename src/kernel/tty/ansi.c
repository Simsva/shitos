#include <kernel/tty.h>
#include "ansi_codes.h"

#include <stdint.h>
#include <features.h>

/* control chars */
#define LEVEL_NONE 0
#define LEVEL_ESC  1
#define LEVEL_CSI  2
#define LEVEL_OSC  3

#define LEN(a) (sizeof(a)/sizeof(a[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const uint32_t color4_to_24[] = {
    0x00000000, 0x00aa0000, 0x0000aa00, 0x00aa5500,
    0x000000aa, 0x00aa00aa, 0x0000aaaa, 0x00aaaaaa,
    0x00555555, 0x00ff5555, 0x0055ff55, 0x00ffff55,
    0x005555ff, 0x00ff55ff, 0x0055ffff, 0x00ffffff,
};

static void ansi_handle_esc(ansi_ctx_t *ctx, uint32_t c);
static void ansi_handle_csi(ansi_ctx_t *ctx, uint32_t c);
static void ansi_sgr(ansi_ctx_t *ctx);
static void ansi_check_cursor(ansi_ctx_t *ctx);
static uint16_t ansi_parse_dec_rev(const char *s, uint8_t i);
static uint32_t ansi_color8_to_24(uint8_t c);

void ansi_init(ansi_ctx_t *ctx, uint16_t w, uint16_t h, uint32_t color_fmt,
               uint32_t color_def_fg, uint32_t color_def_bg,
               ansi_callback_t callback) {
    ctx->callback = callback;
    ctx->text_mode = 0;
    ctx->color_fg = ctx->color_def_fg = color_def_fg;
    ctx->color_bg = ctx->color_def_bg = color_def_bg;
    ctx->color_fmt = color_fmt;
    ctx->esc_level = LEVEL_NONE;
    ctx->width = w;
    ctx->height = h;
    ctx->cur_x = 0;
    ctx->cur_y = 0;
}

/* handle a character in an ANSI context */
void ansi_handle(ansi_ctx_t *ctx, uint32_t c) {
    switch(ctx->esc_level) {
    case LEVEL_NONE: break;
    case LEVEL_ESC:  ansi_handle_esc(ctx, c); return;
    case LEVEL_CSI:  ansi_handle_csi(ctx, c); return;
    case LEVEL_OSC:  ctx->esc_level = LEVEL_NONE; break;
    }

    switch(c) {
    case C0_BS:
        if(ctx->cur_x != 0) ctx->cur_x--;
        break;

    /* Handle HT maybe? */

    case C0_LF:
        ctx->cur_y++;
        __fallthrough;
    case C0_CR:
        ctx->cur_x = 0;
        break;

    case C0_VT: case C0_FF:
        ctx->cur_y++;
        break;

    /* ESC character (begins ESC sequence) */
    case C0_ESC:
        ctx->esc_level = LEVEL_ESC;
        break;

    /* non-handled characters */
    default:
        ctx->callback(ctx, ANSI_OUT_CHAR, c);
        break;
    }

    /* check if we need to scroll */
    ansi_check_cursor(ctx);
}

/* handle single-char escape sequences */
static void ansi_handle_esc(ansi_ctx_t *ctx, uint32_t c) {
    switch(c) {
    case Fe_CSI:
        ctx->esc_level = LEVEL_CSI;
        ctx->args_sz = ctx->arg_buf_i = 0;
        if(0) {
    case Fe_OSC:
        ctx->esc_level = LEVEL_OSC;
        }
        return;

    case Fe_SS2: c = ANSI_ESC_Fe_SS2; goto esc_event;
    case Fe_SS3: c = ANSI_ESC_Fe_SS3; goto esc_event;
    case Fe_DCS: c = ANSI_ESC_Fe_DCS; goto esc_event;
    case Fe_ST:  c = ANSI_ESC_Fe_ST;  goto esc_event;
    case Fe_SOS: c = ANSI_ESC_Fe_SOS; goto esc_event;
    case Fe_PM:  c = ANSI_ESC_Fe_PM;  goto esc_event;
    case Fe_APC: c = ANSI_ESC_Fe_APC;
    esc_event: ctx->callback(ctx, ANSI_OUT_ESC, c); return;
    }
}

static void ansi_handle_csi(ansi_ctx_t *ctx, uint32_t c) {
    uint8_t b;

    if(c >= '0' && c <= '9') {
        if(ctx->arg_buf_i < LEN(ctx->arg_buf))
            ctx->arg_buf[ctx->arg_buf_i++] = c;
        return;
    }
    if(ctx->args_sz < LEN(ctx->args))
       ctx->args[ctx->args_sz++]
            = ansi_parse_dec_rev(ctx->arg_buf, ctx->arg_buf_i);
    ctx->arg_buf_i = 0;

    /* all other valid characters count as separators, not just ';' */
    if(c >= ':' && c <= '?') return;

    ctx->esc_level = LEVEL_NONE;
    switch(c) {
    case CSI_CPL:
        ctx->cur_x = 0;
        __fallthrough;
    case CSI_CUU:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_y = (ctx->cur_y < b)
            ? 0
            : (ctx->cur_y - b);
        break;

    case CSI_CNL:
        ctx->cur_x = 0;
        __fallthrough;
    case CSI_CUD:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_y = (ctx->height-1 - ctx->cur_y < b)
            ? (ctx->height-1)
            : (ctx->cur_y + b);
        break;

    case CSI_CUF:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_x = (ctx->width-1 - ctx->cur_x < b)
            ? (ctx->width-1)
            : (ctx->cur_x + b);
        break;

    case CSI_CUB:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_x = (ctx->cur_x < b)
            ? 0
            : (ctx->cur_x - b);
        break;

    case CSI_CHA:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_x = MIN(ctx->width, b) - 1;
        break;

    case CSI_HVP: /* TODO: should be handled a bit differently */
    case CSI_CUP:
        b = ctx->args_sz > 0 ? ctx->args[0] : 1;
        ctx->cur_y = MIN(ctx->height, b) - 1;

        b = ctx->args_sz > 1 ? ctx->args[1] : 1;
        ctx->cur_x = MIN(ctx->width, b) - 1;
        break;

    case CSI_ED: c = ANSI_ESC_CSI_ED; goto esc_event;
    case CSI_EL: c = ANSI_ESC_CSI_EL;
    esc_event: ctx->callback(ctx, ANSI_OUT_ESC, c); return;

    case CSI_SGR:
        ansi_sgr(ctx);
        break;

    case CSI_SCP:
        ctx->scur_x = ctx->cur_x;
        ctx->scur_y = ctx->cur_y;
        break;

    case CSI_RCP:
        ctx->cur_x = ctx->scur_x;
        ctx->cur_y = ctx->scur_y;
        break;
    }

    ansi_check_cursor(ctx);
}

static void ansi_sgr(ansi_ctx_t *ctx) {
    uint8_t mode = ctx->args_sz == 0 ? 0 : ctx->args[0];

    if(mode >= SGR_FG_COLOR_FIRST && mode <= SGR_FG_COLOR_LAST) {
        ctx->color_fg = ctx->color_fmt == 0
                ? (uint32_t)mode - SGR_FG_COLOR_FIRST
                : color4_to_24[mode - SGR_FG_COLOR_FIRST];
        return;
    }
    if(mode >= SGR_BG_COLOR_FIRST && mode <= SGR_BG_COLOR_LAST) {
        ctx->color_bg = ctx->color_fmt == 0
                ? (uint32_t)mode - SGR_BG_COLOR_FIRST
                : color4_to_24[mode - SGR_BG_COLOR_FIRST];
        return;
    }
    if(mode >= SGR_FG_COLOR_BRIGHT_FIRST && mode <= SGR_FG_COLOR_BRIGHT_LAST) {
        ctx->color_fg = ctx->color_fmt == 0
                ? (uint32_t)mode - SGR_FG_COLOR_BRIGHT_FIRST + 8
                : color4_to_24[mode - SGR_FG_COLOR_BRIGHT_FIRST + 8];
        return;
    }
    if(mode >= SGR_BG_COLOR_BRIGHT_FIRST && mode <= SGR_BG_COLOR_BRIGHT_LAST) {
        ctx->color_bg = ctx->color_fmt == 0
                ? (uint32_t)mode - SGR_BG_COLOR_BRIGHT_FIRST + 8
                : color4_to_24[mode - SGR_BG_COLOR_BRIGHT_FIRST + 8];
        return;
    }

    switch(mode) {
    case SGR_RESET:
        ctx->color_fg = ctx->color_def_fg;
        ctx->color_bg = ctx->color_def_bg;
        ctx->text_mode = 0;
        break;

    case SGR_BOLD:  ctx->text_mode |= ANSI_MODE_BOLD; break;
    case SGR_FAINT: ctx->text_mode |= ANSI_MODE_FAINT; break;
    case SGR_NORMAL_INTENSITY:
        ctx->text_mode &= ~(ANSI_MODE_BOLD|ANSI_MODE_FAINT); break;

    case SGR_ITALIC:  ctx->text_mode |= ANSI_MODE_ITALIC; break;
    case SGR_FRAKTUR: ctx->text_mode |= ANSI_MODE_FRAKTUR; break;
    case SGR_NOT_ITALIC_BLACKLETTER:
        ctx->text_mode &= ~(ANSI_MODE_ITALIC|ANSI_MODE_FRAKTUR); break;

    case SGR_UNDERLINE:     ctx->text_mode |= ANSI_MODE_UNDERLINE; break;
    case SGR_DBL_UNDERLINE: ctx->text_mode |= ANSI_MODE_DBL_UNDERLINE; break;
    case SGR_NOT_UNDERLINE:
        ctx->text_mode &= ~(ANSI_MODE_UNDERLINE|ANSI_MODE_DBL_UNDERLINE); break;

    case SGR_SLOW_BLINK: ctx->text_mode |= ANSI_MODE_SLOW_BLINK; break;
    case SGR_FAST_BLINK: ctx->text_mode |= ANSI_MODE_FAST_BLINK; break;
    case SGR_NOT_BLINK:
        ctx->text_mode &= ~(ANSI_MODE_SLOW_BLINK|ANSI_MODE_FAST_BLINK); break;

    case SGR_INVERT:     ctx->text_mode |=  ANSI_MODE_INVERT; break;
    case SGR_NOT_INVERT: ctx->text_mode &= ~ANSI_MODE_INVERT; break;

    case SGR_CONCEAL:     ctx->text_mode |=  ANSI_MODE_CONCEAL; break;
    case SGR_NOT_CONCEAL: ctx->text_mode &= ~ANSI_MODE_CONCEAL; break;

    case SGR_STRIKE:     ctx->text_mode |=  ANSI_MODE_STRIKE; break;
    case SGR_NOT_STRIKE: ctx->text_mode &= ~ANSI_MODE_STRIKE; break;

    case SGR_FRAMED:    ctx->text_mode |= ANSI_MODE_FRAMED; break;
    case SGR_ENCIRCLED: ctx->text_mode |= ANSI_MODE_ENCIRCLED; break;
    case SGR_NOT_FRAMED_ENCIRCLED:
        ctx->text_mode &= ~(ANSI_MODE_FRAMED|ANSI_MODE_ENCIRCLED); break;

    case SGR_OVERLINE:     ctx->text_mode |=  ANSI_MODE_OVERLINE; break;
    case SGR_NOT_OVERLINE: ctx->text_mode &= ~ANSI_MODE_OVERLINE; break;

    case SGR_SUPERSCRIPT: ctx->text_mode |= ANSI_MODE_SUPERSCRIPT; break;
    case SGR_SUBSCRIPT:   ctx->text_mode |= ANSI_MODE_SUBSCRIPT; break;
    case SGR_NOT_SUPERSCRIPT_SUBSCRIPT:
        ctx->text_mode &= ~(ANSI_MODE_SUPERSCRIPT|ANSI_MODE_SUBSCRIPT); break;

    case SGR_FG_COLOR_DEFAULT:
        ctx->color_fg = ctx->color_def_fg;
        break;
    case SGR_BG_COLOR_DEFAULT:
        ctx->color_bg = ctx->color_def_bg;
        break;

    case SGR_FG_COLOR_SET_8BIT:
        if(ctx->color_fmt == 0 || ctx->args_sz < 3) break;
        if(ctx->args[1] == 5)
            /* 8-bit color */
            ctx->color_fg = ansi_color8_to_24(ctx->args[2]);
        else if(ctx->args[1] == 2 && ctx->args_sz >= 5)
            /* 24-bit color */
            ctx->color_fg = ((uint32_t)ctx->args[2] & 0xff) << 16
                          | ((uint32_t)ctx->args[3] & 0xff) << 8
                          | ((uint32_t)ctx->args[4] & 0xff);
        break;
    case SGR_BG_COLOR_SET_8BIT:
        if(ctx->color_fmt == 0 || ctx->args_sz < 3) break;
        if(ctx->args[1] == 5)
            /* 8-bit color */
            ctx->color_bg = ansi_color8_to_24(ctx->args[2]);
        else if(ctx->args[1] == 2 && ctx->args_sz >= 5)
            /* 24-bit color */
            ctx->color_bg = ((uint32_t)ctx->args[2] & 0xff) << 16
                          | ((uint32_t)ctx->args[3] & 0xff) << 8
                          | ((uint32_t)ctx->args[4] & 0xff);
        break;
    }
}

static void ansi_check_cursor(ansi_ctx_t *ctx) {
    if(ctx->cur_x >= ctx->width)
        ctx->cur_x = 0, ctx->cur_y++;

    if(ctx->cur_y >= ctx->height) {
        ctx->callback(ctx, ANSI_OUT_SCROLL, ctx->cur_y - ctx->height + 1);
        ctx->cur_y = ctx->height-1;
    }
}

static uint16_t ansi_parse_dec_rev(const char *s, uint8_t i) {
    uint16_t prod = 1, out = 0;
    char c;
    while(i) {
        c = s[--i];
        out += prod * (c-'0');
        prod *= 10;
    }
    return out;
}

static uint32_t ansi_color8_to_24(uint8_t c) {
    if(c < 16) return color4_to_24[c];
    uint8_t r, g, b;

    /* grayscale */
    if(c > 231) r = g = b = ((c - 232)*0xff)/23;
    else {
        /* color cube */
        /* c = 16 + 36*r + 6*g + b, 0 <= r, g, b <= 5 */
        c -= 16;
        r = c / 36;
        c -= 36 * r;
        g = c / 6;
        c -= 6 * g;
        b = c;

        r *= 51; g *= 51; b *= 51;
    }

    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}
