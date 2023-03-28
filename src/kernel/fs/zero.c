#include <kernel/fs.h>

#include <kernel/kmem.h>
#include <string.h>

static ssize_t null_read(__unused fs_node_t *node, __unused off_t off, __unused size_t sz, __unused uint8_t *buf) {
    return 0;
}

static ssize_t null_write(__unused fs_node_t *node, __unused off_t off, __unused size_t sz, __unused uint8_t *buf) {
    return 0;
}

static ssize_t zero_read(__unused fs_node_t *node, __unused off_t off, size_t sz, uint8_t *buf) {
    memset(buf, 0, sz);
    return sz;
}

static ssize_t zero_write(__unused fs_node_t *node, __unused off_t off, __unused size_t sz, __unused uint8_t *buf) {
    return 0;
}

static fs_node_t *null_dev_create(void) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    fnode->inode = 0;
    strcpy(fnode->name, "null");
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0666;
    fnode->flags = FS_TYPE_CHAR;
    fnode->read = null_read;
    fnode->write = null_write;
    return fnode;
}

static fs_node_t *zero_dev_create(void) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    fnode->inode = 0;
    strcpy(fnode->name, "zero");
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0666;
    fnode->flags = FS_TYPE_CHAR;
    fnode->read = zero_read;
    fnode->write = zero_write;
    return fnode;
}

void zero_install(void) {
    vfs_mount("/dev/null", null_dev_create());
    vfs_mount("/dev/zero", zero_dev_create());
}
