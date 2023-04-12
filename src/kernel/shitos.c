#include <kernel/tty.h>
#include <kernel/fs.h>
#include <kernel/console.h>
#include <kernel/video.h>
#include <kernel/args.h>
#include <ext2fs/ext2.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/psf.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

struct kernel_args kernel_args;

/* TODO: move */
void ps2hid_install(void);
void ide_init(void);
void dospart_init(void);
void bootpart_init(void);

static void tree_print_fs(tree_item_t item) {
    struct vfs_entry *entry = item;
    printf("%s", entry->name);
    if(entry->fs_type)
        printf("[%s]", entry->fs_type);
    if(entry->file)
        printf(" -> %s : %u", entry->file->name, entry->file->inode);
    putchar('\n');
}

static uint16_t utf8_to_utf16(char *s) {
    uint16_t uc = s[0];
    if(uc & 0x80) {
        /* UTF-8 to UTF-16 */
        if((uc & 0x20) == 0) {
            uc = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
        } else if((uc & 0x10) == 0) {
            uc = ((((s[0] & 0xf) << 6) + (s[1] & 0x3f)) << 6)
                    + (s[2] & 0x3f);
        } else if((uc & 0x8) == 0) {
            uc = ((((((s[0] & 0x7) << 6) + (s[1] & 0x3f)) << 6)
                    + (s[2] & 0x3f)) << 6) + (s[3] & 0x3f);
        } else
            uc = 0;
    }
    return uc;
}

static void print_utf16(psf_file_t *psf, char *s) {
    psf2_hdr_t *hdr = (void *)psf->file;
    uint16_t c = utf8_to_utf16(s);
    void *glyph_bm = psf_get_bitmap(psf, psf_get_glyph_unicode(psf, c));

    for(uint8_t i = 0; i < hdr->height; i++) {
        uint8_t row = ((uint8_t *)glyph_bm)[i];
        for(uint8_t j = 0; j < 8; j++, row <<= 1)
            printf("%c", row & 0x80 ? '#' : ' ');
        printf("\n");
    }
}

void kmain(struct kernel_args *args) {
    memcpy(&kernel_args, args, sizeof kernel_args);

    vfs_install();
    vfs_map_directory("/dev");
    console_install();
    ide_init();
    dospart_init();
    bootpart_init();
    ext2fs_init();
    fb_init();
    if(kernel_args.video_mode == VIDEO_TEXT)
        tm_term_install();
    else
        fb_term_install();
    zero_install();
    random_install();
    ps2hid_install();

    /* TODO: automatically detect devices somehow */
    vfs_mount_type("dospart", "/dev/ada", NULL);
    vfs_mount_type("ext2fs", "/dev/ada2,rw,verbose", "/");
    vfs_mount_type("bootpart", "/dev/ada1,verbose", "/boot");

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    psf_file_t *psf = psf_open("/usr/share/consolefonts/default8x16.psfu");
    psf_generate_utf16_map(psf);

    print_utf16(psf, "ß");
    print_utf16(psf, "å");

    psf_free(psf);

    for(;;) asm volatile("hlt");
}
