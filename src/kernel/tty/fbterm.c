/**
 * @brief Simple ANSI terminal in a linear framebuffer. As of now, it only
 * supports 32bpp.
 */
/* TODO: cursor */
#include <kernel/tty.h>
#include <kernel/psf.h>
#include <kernel/video.h>
#include <kernel/console.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "ansi_codes.h"

#define DEFAULT_COLOR_FG 0xaaaaaa
#define DEFAULT_COLOR_BG 0x000000

static psf_file_t *psf_font = NULL;
static uint32_t font_w, font_h;
static size_t font_sz;
static ansi_ctx_t ansi_ctx;

static void fb_term_scroll(ansi_ctx_t *ctx, uint32_t l);
static void fb_term_clear(ansi_ctx_t *ctx, uint8_t mode);
static void fb_term_clear_line(ansi_ctx_t *ctx, uint8_t mode);
static void fb_term_render_char(uint32_t cur_x, uint32_t cur_y, uint32_t uc, uint32_t fg, uint32_t bg);
static void fb_term_ansi_callback(ansi_ctx_t *ctx, enum ansi_out type, uint32_t uc);
static int fb_term_load_font(ansi_ctx_t *ctx, const char *font_path);
static ssize_t fb_term_output(size_t sz, uint8_t *buf);

static void fb_term_scroll(ansi_ctx_t *ctx, uint32_t l) {
    size_t line_sz = fb_stride * font_h;
    memmove(fb_vid_memory, fb_vid_memory + line_sz * l,
            line_sz * (ctx->height - l));
    memset(fb_vid_memory + line_sz * (ctx->height - l), 0,
           line_sz * l);
}

static void fb_term_clear(ansi_ctx_t *ctx, uint8_t mode) {
    size_t char_h = font_h,
           line_sz = fb_stride * char_h;
    switch(mode) {
    case 0:
        /* clear cursor down */
        memset(fb_vid_memory + (ctx->cur_y+1) * line_sz, 0,
               fb_memsize - (ctx->cur_y+1) * line_sz);
        fb_term_clear_line(ctx, 0);
        break;

    case 1:
        /* clear cursor up */
        memset(fb_vid_memory, 0, ctx->cur_y * line_sz);
        fb_term_clear_line(ctx, 1);
        break;

    case 2: case 3:
        /* clear screen */
        /* 3 clears scrollback buffer too, but we do not have that */
        memset(fb_vid_memory, 0, fb_memsize);
        break;
    }
}

static void fb_term_clear_line(ansi_ctx_t *ctx, uint8_t mode) {
    size_t char_w = (fb_depth/8) * font_w,
           char_h = font_h,
           line_sz = fb_stride * char_h;
    switch(mode) {
    case 0:
        /* clear cursor right */
        for(size_t i = 0; i < char_h; i++)
            memset(fb_vid_memory + ctx->cur_x * char_w + ctx->cur_y * line_sz
                   + fb_stride * i, 0, (fb_stride - ctx->cur_x * char_w));
        break;

    case 1:
        /* clear cursor left */
        for(size_t i = 0; i < char_h; i++)
            memset(fb_vid_memory + ctx->cur_y * line_sz + fb_stride * i,
                   0, (ctx->cur_x+1) * char_w);
        break;

    case 2:
        /* clear line */
        memset(fb_vid_memory + ctx->cur_y * line_sz, 0, line_sz);
        break;
    }
}

static void fb_term_render_char(uint32_t cur_x, uint32_t cur_y, uint32_t uc, uint32_t fg, uint32_t bg) {
    uint32_t *pixels = (uint32_t *)fb_vid_memory;
    uint32_t *start = pixels + cur_x*font_w + cur_y*font_h*fb_width;

    uint8_t *bm = psf_get_bitmap(psf_font, psf_get_glyph(psf_font, uc));

    uint8_t mask, mask_i;
    uint32_t x, y;
    for(y = 0; y < font_h; y++) {
        mask = *bm++, mask_i = 0;
        for(x = 0; x < font_w; x++) {
            if(mask_i > 7) mask = *bm++, mask_i = 0;
            start[x + y*fb_width] = (mask & 0x80) ? fg : bg;
            mask <<= 1, mask_i++;
        }
    }
}

static void fb_term_ansi_callback(ansi_ctx_t *ctx, enum ansi_out type, uint32_t uc) {
    if(type == ANSI_OUT_CHAR) {
        /* ignore DEL */
        if(uc == '\177') return;

        /* C0 control chars */
        if(uc < ' ') {
            switch(uc) {
            case C0_HT:
                ctx->cur_x = (ctx->cur_x+1 + 7) & ~7;
                break;
            }
            return;
        }

        fb_term_render_char(ctx->cur_x, ctx->cur_y, uc,
                            ctx->color_fg, ctx->color_bg);
        ctx->cur_x++;
    } else if(type == ANSI_OUT_ESC) {
        switch(uc) {
        case ANSI_ESC_CSI_ED:
            fb_term_clear(ctx, ctx->args_sz == 0 ? 0 : ctx->args[0]);
            break;
        case ANSI_ESC_CSI_EL:
            fb_term_clear_line(ctx, ctx->args_sz == 0 ? 0 : ctx->args[0]);
            break;
        }
    } else if(type == ANSI_OUT_SCROLL) {
        fb_term_scroll(ctx, uc);
    }
}

static int fb_term_load_font(ansi_ctx_t *ctx, const char *font_path) {
    if(psf_font) psf_free(psf_font);

    psf_font = psf_open(font_path);
    if(!psf_font) return 1;
    psf_generate_utf16_map(psf_font);

    font_w = psf_get_width(psf_font);
    font_h = psf_get_height(psf_font);
    font_sz = psf_get_size(psf_font);

    ctx->width = fb_width/font_w;
    ctx->height = fb_height/font_h;

    return 0;
}

static ssize_t fb_term_output(size_t sz, uint8_t *buf) {
    size_t i;
    for(i = 0; i < sz; i++) ansi_handle(&ansi_ctx, *buf++);
    return i;
}

int printf(const char *, ...);
void fb_term_install(const char *font_path) {
    /* only supports 32bpp */
    if(fb_depth != 32) return;

    /* width and height set according to font */
    ansi_init(&ansi_ctx, 0, 0, 2, DEFAULT_COLOR_FG, DEFAULT_COLOR_BG,
              fb_term_ansi_callback);
    if(fb_term_load_font(&ansi_ctx, font_path)) return;

    console_set_output(fb_term_output);

    printf("fbterm: %ux%u\n", ansi_ctx.width, ansi_ctx.height);
}
