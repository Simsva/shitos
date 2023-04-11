#include <kernel/tty/tm.h>
#include <kernel/fs.h>
#include <kernel/args.h>
#include <ext2fs/ext2.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/kmem.h>
#include <sys/stat.h>

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

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;
    memcpy(&kernel_args, args, sizeof kernel_args);

    vfs_install();
    vfs_map_directory("/dev");
    console_install();
    zero_install();
    random_install();
    ps2hid_install();
    ide_init();
    dospart_init();
    bootpart_init();
    ext2fs_init();

    /* TODO: automatically detect devices somehow */
    vfs_mount_type("dospart", "/dev/ada", NULL);
    vfs_mount_type("ext2fs", "/dev/ada2,rw,verbose", "/");
    vfs_mount_type("bootpart", "/dev/ada1,verbose", "/boot");

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    for(;;) asm volatile("hlt");
}
