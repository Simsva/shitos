#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <kernel/tty/tm.h>
#include <string.h>

/**
 * VFS interface for the kernel console
 * For now it writes all output in VGA text mode
 */

fs_node_t *console_dev = NULL;

static ssize_t console_write(__unused fs_node_t *node, __unused off_t off, size_t sz, uint8_t *buf) {
    size_t i;
    for(i = 0; i < sz; i++) tm_putc(*buf++);
    return i;
}

static fs_node_t *console_dev_create(void) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    fnode->inode = 0;
    strcpy(fnode->name, "console");
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0660;
    fnode->flags = FS_TYPE_CHAR;
    fnode->write = console_write;
    return fnode;
}

void console_install(void) {
    console_dev = console_dev_create();
    vfs_mount("/dev/console", console_dev);
}
