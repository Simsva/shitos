#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/pci.h>

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

/* detect some Intel IDE controller device */
static void find_ata_pci(pci_device_t dev, uint16_t vnid, uint16_t dvid, void *extra) {
    if(vnid == 0x8086 && (dvid == 0x7010 || dvid == 0x7111))
        *(pci_device_t *)extra = dev;
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

    pci_device_t ata_dev = (pci_device_t){ .raw = 0 };
    pci_scan(find_ata_pci, -1, &ata_dev);

    printf("IDE: bus:%u slot:%u func:%u\n", ata_dev.bus, ata_dev.slot, ata_dev.func);

    for(;;) asm("hlt");
}
