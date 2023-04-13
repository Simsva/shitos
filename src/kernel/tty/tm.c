#include <kernel/tty.h>
#include <kernel/console.h>
#include <kernel/video.h>
#include <kernel/args.h>
#include <kernel/arch/i386/ports.h>

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "ansi_codes.h"

#define TM_WIDTH  (80)
#define TM_HEIGHT (25)

#define DEFAULT_COLOR_FG 0x7 /* in ANSI/SGR format */
#define DEFAULT_COLOR_BG 0x0

#define tm_memory ((uint16_t *)fb_vid_memory)
static ansi_ctx_t ansi_ctx;
static const uint8_t tm_sgr_to_vga[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static void tm_cursor_update(ansi_ctx_t *ctx);
static void tm_scroll(ansi_ctx_t *ctx, uint8_t lines);
static void tm_clear(ansi_ctx_t *ctx, uint8_t mode);
static void tm_clear_line(ansi_ctx_t *ctx, uint8_t mode);
static uint8_t tm_from_ansi_color(uint8_t fg, uint8_t bg);
static void tm_ansi_callback(ansi_ctx_t *ctx, enum ansi_out type, uint32_t uc);
static ssize_t tm_console_write(size_t sz, uint8_t *buf);

static void tm_cursor_update(ansi_ctx_t *ctx) {
    /* update blinking cursor */
    uint16_t tm_cursor = ctx->cur_x + ctx->cur_y*ctx->width;
    outportb(0x03d4, 14);
    outportb(0x03d5, tm_cursor>>8);
    outportb(0x03d4, 15);
    outportb(0x03d5, tm_cursor & 0xff);
}

static void tm_scroll(ansi_ctx_t *ctx, uint8_t lines) {
    memcpy(tm_memory, tm_memory + lines*ctx->width,
           (ctx->height-lines)*ctx->width*sizeof(tm_memory[0]));
    memset(tm_memory + (ctx->height-lines)*ctx->width, 0,
           lines*ctx->width*sizeof(tm_memory[0]));
}

static void tm_clear(ansi_ctx_t *ctx, uint8_t mode) {
    switch(mode) {
    case 0:
        /* clear cursor down */
        memset(tm_memory + ctx->cur_x + ctx->cur_y*ctx->width, 0,
               (ctx->width*ctx->height - (ctx->cur_x + ctx->cur_y*ctx->width))
               * sizeof(tm_memory[0]));
        break;

    case 1:
        /* clear cursor up */
        memset(tm_memory, 0, (ctx->cur_x + ctx->cur_y*ctx->width + 1)
               * sizeof(tm_memory[0]));
        break;

    case 2: case 3:
        /* clear screen */
        /* 3 clears scrollback buffer too, but we do not have that */
        memset(tm_memory, 0,
               ctx->width*ctx->height * sizeof(tm_memory[0]));
        break;
    }
}

static void tm_clear_line(ansi_ctx_t *ctx, uint8_t mode) {
    switch(mode) {
    case 0:
        /* clear cursor right */
        memset(tm_memory + ctx->cur_x + ctx->cur_y*ctx->width, 0,
               (ctx->width - ctx->cur_x) * sizeof(tm_memory[0]));
        break;

    case 1:
        /* clear cursor left */
        memset(tm_memory + ctx->cur_y*ctx->width, 0,
               (ctx->cur_x+1) * sizeof(tm_memory[0]));
        break;

    case 2:
        /* clear line */
        memset(tm_memory + ctx->cur_y*ctx->width, 0,
               ctx->width * sizeof(tm_memory[0]));
        break;
    }
}

static uint8_t tm_from_ansi_color(uint8_t fg, uint8_t bg) {
    return ((fg < 8) ? (tm_sgr_to_vga[fg]) : (tm_sgr_to_vga[fg - 8] + 8))
         | ((bg < 8) ? (tm_sgr_to_vga[bg]) : (tm_sgr_to_vga[bg - 8] + 8)) << 4;
}

static void tm_ansi_callback(ansi_ctx_t *ctx, enum ansi_out type, uint32_t uc) {
    if(type == ANSI_OUT_CHAR) {
        uint8_t c = (uint8_t)uc;

        /* ignore DEL */
        if(c == '\177') return;

        /* C0 control chars */
        if(c < ' ') {
            switch(c) {
            case C0_HT:
                ctx->cur_x = (ctx->cur_x+1 + 7) & ~7;
                break;
            }
            return;
        }

        uint8_t real_color = tm_from_ansi_color(ctx->color_fg, ctx->color_bg);
        if(ctx->text_mode & ANSI_MODE_INVERT)
            real_color = (real_color>>4) | (real_color<<4);
        if(ctx->text_mode & ANSI_MODE_CONCEAL)
            real_color = (real_color&0xf0) | ((real_color&0xf0)>>4);

        tm_memory[ctx->cur_x + ctx->cur_y*ctx->width] = real_color<<8 | c;
        ctx->cur_x++;
    } else if(type == ANSI_OUT_ESC) {
        switch(uc) {
        case ANSI_ESC_CSI_ED:
            tm_clear(ctx, ctx->args_sz == 0 ? 0 : ctx->args[0]);
            break;
        case ANSI_ESC_CSI_EL:
            tm_clear_line(ctx, ctx->args_sz == 0 ? 0 : ctx->args[0]);
            break;
        }

    } else if(type == ANSI_OUT_SCROLL) {
        tm_scroll(ctx, (uint8_t)uc);
    }

    tm_cursor_update(ctx);
}

static ssize_t tm_console_write(size_t sz, uint8_t *buf) {
    size_t i;
    for(i = 0; i < sz; i++) ansi_handle(&ansi_ctx, *buf++);
    return i;
}

void tm_term_install(void) {
    ansi_init(&ansi_ctx, TM_WIDTH, TM_HEIGHT, 0,
              DEFAULT_COLOR_FG, DEFAULT_COLOR_BG, tm_ansi_callback);
    ansi_ctx.cur_x = kernel_args.tm_cursor % TM_WIDTH;
    ansi_ctx.cur_y = kernel_args.tm_cursor / TM_WIDTH;

    console_set_output(tm_console_write);
}
