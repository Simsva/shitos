#include <sys/utils.h>
#include <boot/def.h>
#include <string.h>
#include <elf.h>

#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "isr.h"
#include "tm_io.h"
#include "scan_code.h"
#include "partition.h"

#include "boot_opts.h"

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

/* video modes */
enum video_opt {
OPT_VIDEO_TEXT,
OPT_VIDEO_QEMU,
N_OPT_VIDEO,
};
/* TODO: auto-mode? */
#define VIDEO_OPT_DEFAULT OPT_VIDEO_QEMU
const char *opt_video_str[N_OPT_VIDEO] = { "Text Mode", "QEMU", };

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

extern void call_kernel(void *, size_t, struct kernel_args);

/* global vars */
void *global_esp;

uint32_t ticks = 0;
uint8_t menu_current = 0; /* 0 for main, 1 for options */
uint8_t boot_options = BOOT_OPTS_DEFAULT;
enum video_opt video_opt = VIDEO_OPT_DEFAULT;
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
3. %H0fV%H07erbose: %h%s%H07\n\
4. %H0fG%H07raphics: %s";

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
              OPT_FMT(HAS_OPT(BOOT_OPT_VERBOSE)),
              opt_video_str[video_opt]);

    tm_cursor_set(0, 24);
    tm_color = 0x07;
}

/* NOTE: runs at 18.222 Hz by default */
void timer_handler(__attribute__((unused)) struct int_regs *r) {
    ++ticks;
}

/* NOTE: ignores SCAN_EXTENDED */
void kb_handler(__attribute__((unused)) struct int_regs *r) {
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
            return; /* will never happen */

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
            boot_options = BOOT_OPTS_DEFAULT;
            video_opt = VIDEO_OPT_DEFAULT;
            break;

        /* Boot Options: Verbose */
        case SCAN1_3: case SCAN1_V:
            boot_options ^= BOOT_OPT_VERBOSE;
            break;

        /* Boot Options: Graphics Mode */
        case SCAN1_4: case SCAN1_G:
            video_opt = (video_opt + 1) % N_OPT_VIDEO;
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

/* GCC 12 does not like this */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
void boot(void) {
    struct partition_entry *parts =
        (struct partition_entry *)(MEM_MBR+0x1be);
    void *elf_off;
    Elf32_Ehdr *elf_hdr;
    Elf32_Phdr *elf_phdr;
    void *kernel_entry;
    uint16_t i;
    uint8_t drive_num = *((uint8_t *)MEM_DRV);
    int8_t err;

    /* uninstall keyboard handler */
    irq_handler_uninstall(1);

    tm_cursor_set(0, 24);
    tm_line_reset = 0;
    tm_color = 0x07;
    tm_puts("Booting...");

    /* read MBR at 0x300 */
    xread(0, 0, MEM_MBR, drive_num, 1);
    if(*((uint16_t *)(MEM_MBR+0x1fe)) != 0xaa55) {
        tm_color = 0x4f;
        tm_puts("Invalid MBR found");
        goto halt;
    }

    for(i = 0; i < 4; ++i) {
        tm_printf("partition %u: boot:%x type:%x lba:%u\n",
                  i, parts[i].boot, parts[i].type, parts[i].start_lba);
        if(parts[i].type != 0x83) continue;

        err = partition_ext2_parse(parts+i, drive_num, (void **)&elf_off);
        switch(err) {
        case PARSE_EXT2_SUCCESS:
            tm_puts("Found kernel ELF");
            goto found;

        case PARSE_EXT2_NOTEXT2:
            tm_puts("Not an EXT2 partition");
            break;

        case PARSE_EXT2_NOFILE:
            tm_puts("No kernel found in partition");
            break;

        case PARSE_EXT2_TOOBIG:
            tm_color = 0x4f;
            tm_puts("KERNEL FILE IS TOO BIG");
            goto halt;
        }
    }
    tm_puts("No EXT2 partition found");
    goto halt;

    /* TODO: load kernel */
found:
    elf_hdr = elf_off;
    if(memcmp(elf_hdr->e_ident, ELFMAG, SELFMAG)) {
        tm_color = 0x4f;
        tm_puts("Malformed kernel ELF!");
        goto halt;
    }

    if(HAS_OPT(BOOT_OPT_VERBOSE))
        tm_puts("Program Header:");
    elf_phdr = elf_off + elf_hdr->e_phoff;
    for(i = 0; i < elf_hdr->e_phnum; ++i) {
        if(HAS_OPT(BOOT_OPT_VERBOSE))
            tm_printf("  type:%u off:0x%x vaddr:0x%x paddr:0x%x align:%u\n\
    filesz:0x%x memsz:0x%x flags:%c%c%c\n",
                      elf_phdr->p_type, elf_phdr->p_offset, elf_phdr->p_vaddr,
                      elf_phdr->p_paddr, elf_phdr->p_align, elf_phdr->p_filesz,
                      elf_phdr->p_memsz, (elf_phdr->p_flags&PF_R) ? 'r' : '-',
                      (elf_phdr->p_flags&PF_W) ? 'w' : '-',
                      (elf_phdr->p_flags&PF_X) ? 'x' : '-');

        if(elf_phdr->p_type == PT_LOAD) {
            memcpy((void *)elf_phdr->p_paddr, elf_off + elf_phdr->p_offset,
                   elf_phdr->p_filesz);
            memset((void *)elf_phdr->p_paddr + elf_phdr->p_filesz, 0,
                   elf_phdr->p_memsz - elf_phdr->p_filesz);
        }

        elf_phdr = (void *)elf_phdr + elf_hdr->e_phentsize;
    }

    tm_puts("Calling kmain");
    kernel_entry = (void *)elf_hdr->e_entry;

    /* call kmain with a known environment */
    struct kernel_args args = {
        .tm_cursor = tm_cursor,
        .boot_options = boot_options,
        .drive_num = drive_num,
        .video_x = 0,   /* TODO: be able to change this maybe? */
        .video_y = 0,
        .video_depth = 0,
        .video_memory = 0,
        .video_mode = video_opt == OPT_VIDEO_QEMU ? VIDEO_QEMU : VIDEO_TEXT,
    };

    call_kernel(kernel_entry, sizeof args, args);

halt:
    for(;;) asm("hlt");
}
#pragma GCC diagnostic pop

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
