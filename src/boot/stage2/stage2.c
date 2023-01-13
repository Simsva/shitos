#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "isr.h"
#include "io.h"
#include "tm_io.h"
#include "scan_code.h"
#include "partition.h"

#ifndef asm
# define asm __asm__ volatile
#endif

/* menu screen offsets */
#define MENU_BRAND_X  2
#define MENU_BRAND_Y  2
#define MENU_BOX_X    1
#define MENU_BOX_Y    10
#define MENU_BOX_H    10    /* excluding the top and bottom */
#define MENU_LOGO_X   44
#define MENU_LOGO_Y   7

/* boot option utils */
#define OPT_COLOR_ON  0x0a
#define OPT_COLOR_OFF 0x0c
#define OPT_FMT(x)    ((x) ? OPT_COLOR_ON : OPT_COLOR_OFF),\
                      ((x) ? menu_opt_on  : menu_opt_off)
#define HAS_OPT(x)    (boot_options&x)

/* boot options */
#define OPTS_DEFAULT 0x01
#define OPT_VERBOSE  0x01

/* timer */
#define TPS              18    /* 18.222 ~= 18 */
#define AUTOBOOT_TIMEOUT 10

/* forward declarations */
void draw_menu_skeleton(void);
void draw_menu_clear(void);
void draw_menu_main(void);
void draw_menu_opts(void);

void timer_handler(struct int_regs *);
void kb_handler(struct int_regs *);

void reboot(void);
void boot(void);

/* global vars */
void *global_esp;

uint32_t ticks = 0;
uint8_t menu_current = 0; /* 0 for main, 1 for options */
uint8_t boot_options = OPTS_DEFAULT;
uint8_t autoboot = 1, /* set to 0 to stop autoboot */
        booting = 0;  /* is currently booting */

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
    draw_menu_clear();
    menu_current = 0;

    tm_cursor_set(MENU_BOX_X+3, MENU_BOX_Y+2);
    tm_line_reset = MENU_BOX_X+3;
    tm_color = 0x07;
    tm_printf(menu_main);

    tm_cursor_set(0, 24);
    tm_color = 0x07;
}

void draw_menu_opts(void) {
    draw_menu_clear();
    menu_current = 1;

    tm_cursor_set(MENU_BOX_X+3, MENU_BOX_Y+2);
    tm_line_reset = MENU_BOX_X+3;
    tm_color = 0x07;
    tm_printf(menu_opts,
              OPT_FMT(boot_options&OPT_VERBOSE));

    tm_cursor_set(0, 24);
    tm_color = 0x07;
}

/* NOTE: runs at 18.222 Hz by default */
void timer_handler(struct int_regs *r) {
    ++ticks;
}

/* NOTE: ignores SCAN_EXTENDED */
void kb_handler(struct int_regs *r) {
    uint8_t sc;
    uint16_t cursor;

    /* save cursor */
    cursor = tm_cursor;
    sc = inb(0x60);

    /* if key is released */
    if(sc&0x80) return;
    /* disable autoboot on any key press (not just space) */
    autoboot = 0;

    if(menu_current == 0) {
        /* main menu */
        switch(sc) {
        /* Boot ShitOS */
        case SCAN1_1: case SCAN1_B: case SCAN1_ENTER:
            /* skip autoboot timer and continue booting */
            booting = 1;
            return;

        /* Escape to loader prompt */
        case SCAN1_2: case SCAN1_ESCAPE:
            tm_cursor_set(0, 23);
            tm_color = 0x07;
            tm_puts("Prompt NYI");

            /* restore cursor */
            tm_cursor = cursor;
            return;

        /* Reboot */
        case SCAN1_3: case SCAN1_R:
            reboot();

        /* Boot Options */
        case SCAN1_4: case SCAN1_O:
            menu_current = 1;
            break;
        }
    } else {
        /* options menu */
        switch(sc) {
        /* Back to main menu */
        case SCAN1_1: case SCAN1_BACKSPACE:
            menu_current = 0;
            break;

        /* Load System Defaults */
        case SCAN1_2: case SCAN1_D:
            boot_options = OPTS_DEFAULT;
            break;

        /* Boot Options: Verbose */
        case SCAN1_3: case SCAN1_V:
            boot_options ^= OPT_VERBOSE;
            break;
        }
    }

    /* redraw menu */
    if(menu_current == 0) draw_menu_main();
    else                  draw_menu_opts();

    /* restore cursor */
    tm_cursor = cursor;
}

/* pulses the CPU's RESET pin through the keyboard controller */
void reboot(void) {
    uint8_t temp;

    asm("cli");

    /* wait for keyboard buffers to be empty */
    do {
      temp = inb(0x64);
    } while(temp & 0x02);

    /* pulse CPU RESET pin */
    outb(0x64, 0xfe);
    for(;;) asm("hlt");
}

void boot(void) {
    struct partition_entry *parts =
        (struct partition_entry *)(MEM_MBR+0x1be);
    uint8_t drive_num = *((uint8_t *)MEM_DRV);
    uint8_t i;
    int8_t err;

    /* uninstall keyboard handler */
    irq_handler_uninstall(1);

    tm_cursor_set(0, 24);
    tm_line_reset = 0;
    tm_color = 0x07;
    tm_puts("Booting...");

    /* NOTE: only for testing v86, will remove later */
    /* v86.ctl = 0; */
    /* v86.addr = 0x10; */
    /* v86.eax = 0x0013; */
    /* v86int(); */

    /* for(uint16_t i = 0; i < 320*200; ++i) */
    /*     ((uint8_t *)0xa0000)[i] = i/320; */

    /* TODO: move all this to before rendering the menu */

    /* read MBR at 0x300 */
    xread(0, 0, MEM_MBR, drive_num, 1);

    if(HAS_OPT(OPT_VERBOSE))
        tm_printf("hacker? %x\n", *((uint16_t *)(MEM_MBR+0x1fe)));
    for(i = 0; i < 4; ++i) {
        if(HAS_OPT(OPT_VERBOSE))
            tm_printf("boot:%x type:%x lba:%u\n",
                    parts[i].boot, parts[i].type, parts[i].start_lba);
        if(parts[i].type != 0x83) continue;

        err = partition_ext2_parse(parts+i, drive_num);
        switch(err) {
        case PARSE_EXT2_SUCCESS:
            tm_puts("Success");
            goto found;

        case PARSE_EXT2_NOTEXT2:
            tm_puts("Not an EXT2 partition");
            break;
        }
    }
    tm_puts("No EXT2 partition found");
    for(;;) asm("hlt");

    /* TODO: load kernel */
found:
    for(;;) asm("hlt");
}

/* bootloader main */
void bmain(void *esp) {
    global_esp = esp;

    tm_init();
    gdt_install();
    idt_install();
    irq_install();
    isrs_install();

    irq_handler_install(0, timer_handler);
    irq_handler_install(1, kb_handler);

    asm("sti");

    draw_menu_skeleton();
    draw_menu_main();

    uint32_t start = ticks, dt = 0;
    while(autoboot) {
        dt = ticks - start;
        tm_cursor_set(2, MENU_BOX_Y+MENU_BOX_H+2);
        tm_printf("Autoboot in %u seconds. %H0f[Space]%H07 to pause ",
                  AUTOBOOT_TIMEOUT - (dt / TPS));

        if(dt >= AUTOBOOT_TIMEOUT*TPS) {
            booting = 1;
            break;
        }
    }
    tm_cursor_set(2, MENU_BOX_Y+MENU_BOX_H+2);
    for(uint8_t i = 0; i<40; ++i) tm_putc(' ');

    while(!booting) asm("hlt");

    boot();
}
