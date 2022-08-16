#include "string.h"
#include "stdint.h"

#ifndef asm
# define asm __asm__ volatile
#endif

#define PALETTE_MASK  0x03c6
#define PALETTE_READ  0x03c7
#define PALETTE_WRITE 0x03c8
#define PALETTE_DATA  0x03c9

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT)

#define COLOR(_r, _g, _b) ((uint8_t)(\
    (((_r) & 0x7) << 5) | \
    (((_g) & 0x7) << 2) | \
    (((_b) & 0x3) << 0)))

/* magic address */
static uint8_t *fb = (uint8_t *)0xa0000;

static inline void outportb(uint16_t port, uint8_t data) {
  asm("outb %1, %0" : : "dN"(port), "a"(data));
}

void screen_init() {
  outportb(PALETTE_MASK, 0xff);
  outportb(PALETTE_WRITE, 0x00);
  for(uint8_t i = 0; i < 255; i++) {
    outportb(PALETTE_DATA, (((i >> 5) & 0x7) * (256 / 8)) / 4);
    outportb(PALETTE_DATA, (((i >> 2) & 0x7) * (256 / 8)) / 4);
    outportb(PALETTE_DATA, (((i >> 0) & 0x3) * (256 / 4)) / 4);
  }

  /* color 255 = white */
  outportb(PALETTE_DATA, 0x3f);
  outportb(PALETTE_DATA, 0x3f);
  outportb(PALETTE_DATA, 0x3f);
}

void _main(uint32_t magic) {
  screen_init();
  for(uint32_t i = 0; i < SCREEN_SIZE; i++)
    /* will automatically loop back to 0 */
    fb[i] = i / SCREEN_WIDTH;

  for(;;);
}
