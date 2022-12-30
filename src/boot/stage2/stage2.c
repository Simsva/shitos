#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "isr.h"
#include "tm_io.h"

#ifndef asm
# define asm __asm__ volatile
#endif

#define MENU_BRAND_X  2
#define MENU_BRAND_Y  2
#define MENU_BOX_X    1
#define MENU_BOX_Y    10
#define MENU_BOX_H    10    /* excluding the top and bottom */
#define MENU_LOGO_X   44
#define MENU_LOGO_Y   7

#define OPT_ON_COLOR  0x0a
#define OPT_OFF_COLOR 0x0c
#define OPT_COLOR(x)  ((x) ? OPT_ON_COLOR : OPT_OFF_COLOR),\
        ((x) ? menu_opt_on : menu_opt_off)
#define BOOT_VERBOSE  0x1

void *global_esp;
uint32_t ticks = 0;
uint8_t menu_current = 0; /* 0 for main, 1 for options */
uint8_t boot_options_default = 0x0,
        boot_options = 0x0;

const char *brand = "\
  _____  _      _  _     ____    _____\n\
 / ____|| |    (_)| |   / __ \\  / ____|\n\
| (___  | |__   _ | |_ | |  | || (___\n\
 \\___ \\ | '_ \\ | || __|| |  | | \\___ \\\n\
 ____) || | | || || |_ | |__| | ____) |\n\
|_____/ |_| |_||_| \\__| \\____/ |_____/";

const char *logo = "\
              .^~!\n\
            .~!: ~~.\n\
           :!:    :~~^:\n\
          :?         :^~~.\n\
       ^~~7J7~^.        :7.\n\
     ~7^.   .:~!77!~^:::.77:.\n\
    !7           .^~!7777!^:J:\n\
    Y                      ^P?!^\n\
  :!5?^:....  .....::^^~!7??^.:7?\n\
 ~J:.^~~!!!!!7777777!!~^:.      7?\n\
.5.                              5:\n\
.5.                              5^\n\
 ~J^.                      ..:~!7~\n\
  .~!!!!!~~~~~~~~~~~~!!!!!!!!~^.\n\
      ...::::::::::::....";

const char *menu_main = "\
1. %H0fB%H07oot ShitOS %H0f[Enter]%H07\n\
2. %H0fEsc%H07ape to loader prompt\n\
3. %H0fR%H07eboot\n\
4. Boot %H0fO%H07ptions";

const char *menu_opt_on = "ON",
           *menu_opt_off = "off",
           *menu_opts = "\
1. Back to main menu %H0f[Backspace]%H07\n\
2. Load System %H0fD%H07efaults\n\n\n\
Boot Options:\n\
3. %H0fV%H07erbose: %h%s";

void draw_menu_skeleton(void) {
    /* draw FIGlet name */
    tm_cursor_set(MENU_BRAND_X, MENU_BRAND_Y);
    tm_line_reset = MENU_BRAND_X;
    tm_color = 0x07;
    tm_puts(brand);

    /* draw box */
    tm_cursor_set(MENU_BOX_X, MENU_BOX_Y);
    tm_line_reset = MENU_BOX_X;
    tm_puts("+---------- Welcome to ShitOS ----------+");
    for(uint8_t j = 0; j < 2; ++j) {
        for(uint8_t i = 0; i < MENU_BOX_H; ++i)
            tm_puts("|");
        tm_cursor_set(MENU_BOX_X+40, MENU_BOX_Y+1);
        tm_line_reset = MENU_BOX_X+40;
    }
    tm_cursor_set(MENU_BOX_X, MENU_BOX_Y+MENU_BOX_H+1);
    tm_puts("+---------------------------------------+");

    /* draw logo */
    tm_cursor_set(MENU_LOGO_X, MENU_LOGO_Y);
    tm_line_reset = MENU_LOGO_X;
    tm_color = 0x06;
    tm_puts(logo);
}

void draw_menu_clear(void) {
    tm_cursor_set(MENU_BOX_X+3, MENU_BOX_Y+2);
    tm_line_reset = MENU_BOX_X+3;
    for(uint8_t y = 0; y < MENU_BOX_H-1; ++y) {
        for(uint8_t x = 0; x < 35; ++x)
            tm_putc(' ');
        tm_putc('\n');
    }
}

void draw_menu_main(void) {
    tm_cursor_set(MENU_BOX_X+3, MENU_BOX_Y+2);
    tm_line_reset = MENU_BOX_X+3;
    tm_color = 0x07;
    tm_printf(menu_main);

    tm_cursor_set(0, 24);
    tm_color = 0x07;
}

void draw_menu_opts(void) {
    tm_cursor_set(MENU_BOX_X+3, MENU_BOX_Y+2);
    tm_line_reset = MENU_BOX_X+3;
    tm_color = 0x07;
    tm_printf(menu_opts,
              OPT_COLOR(boot_options&BOOT_VERBOSE));

    tm_cursor_set(0, 24);
    tm_color = 0x07;
}

void timer_test(struct int_regs *r) {
    /* will print approximately once every second */
    /* (runs at 18.222 Hz) */
    /* if(ticks++ % 18 == 0) */
    /*     tm_puts("tick"); */

    ++ticks;
}

/* bootloader main */
void bmain(void *esp) {
    global_esp = esp;

    tm_init();
    gdt_install();
    idt_install();
    irq_install();
    isrs_install();

    irq_handler_install(0, timer_test);

    asm("sti");

    draw_menu_skeleton();
    draw_menu_main();

    uint32_t start = ticks, dt = 0;
    while(dt < 18*9) {
        dt = ticks - start;
        tm_cursor_set(2, MENU_BOX_Y+MENU_BOX_H+2);
        tm_printf("Autoboot in %u seconds. %H0f[Space]%H07 to pause",
                  9 - (dt / 18));

        if(dt % 36) continue;

        /* I have no idea why this causes the cursor to appear but it does */
        draw_menu_clear();
        switch(menu_current) {
        case 0:
            draw_menu_main();
            menu_current = 1;
            break;
        case 1:
            boot_options = !boot_options;
            draw_menu_opts();
            menu_current = 0;
            break;
        }
    }
    /* tm_cursor_set(2, MENU_BOX_Y+MENU_BOX_H+2); */
    /* for(uint8_t i = 0; i<39; ++i) tm_putc(' '); */

    /* proceed to autoboot */

    for(;;) asm("hlt");
}
