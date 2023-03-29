#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <kernel/pipe.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

/* TODO: move */
void ps2hid_install(void);

static void tree_print_fs(tree_item_t item) {
    struct vfs_entry *entry = item;
    if(entry->file)
        printf("%s -> %p : %u\n", entry->name, entry->file->device, entry->file->inode);
    else
        printf("%s\n", entry->name);
}

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    vfs_install();
    vfs_map_directory("/dev");
    zero_install();
    random_install();
    console_install();
    ps2hid_install();

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    fs_node_t *kbd_pipe = kopen("/dev/kbd", 0);
    uint8_t buf;
    for(;;) {
        if(pipe_size(kbd_pipe)) {
            fs_read(kbd_pipe, 0, 1, &buf);
            printf("%02X ", buf);
        }
    }

    /* unreachable */
    fs_close(kbd_pipe);
    kfree(kbd_pipe);

    for(;;) asm("hlt");
}
