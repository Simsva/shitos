#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <string.h>

/**
 * VFS interface for the kernel console
 * For now it writes all output in VGA text mode
 */

fs_node_t *console_dev = NULL;

static ssize_t (*console_output)(size_t, uint8_t *) = NULL;
static uint8_t tmp_buffer[1024], *buffer_head = tmp_buffer;

static ssize_t console_write(__unused fs_node_t *node, __unused off_t off, size_t sz, uint8_t *buf) {
    if(console_output) return console_output(sz, buf);

    size_t i = 0;
    while(buffer_head < tmp_buffer + sizeof tmp_buffer && i < sz)
        *buffer_head++ = *buf++, i++;
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
    fnode->flags = FS_FLAG_IFCHR;
    fnode->write = console_write;
    return fnode;
}

void console_set_output(ssize_t (*output)(size_t, uint8_t *)) {
    console_output = output;

    if(console_output && buffer_head != tmp_buffer) {
        console_output(buffer_head - tmp_buffer, tmp_buffer);
        buffer_head = tmp_buffer;
    }
}

void console_install(void) {
    console_dev = console_dev_create();
    vfs_mount("/dev/console", console_dev);
}
