#include <kernel/fs.h>

#include <kernel/kmem.h>
#include <string.h>

struct fs_node *fs_root = NULL;

ssize_t fs_read(struct fs_node *node, off_t off, size_t sz, uint8_t *buf) {
    if(node->read) return node->read(node, off, sz, buf);
    else return -1;
}

ssize_t fs_write(struct fs_node *node, off_t off, size_t sz, uint8_t *buf) {
    if(node->write) return node->write(node, off, sz, buf);
    else return -1;
}

/* TODO: figure out what this is even supposed to do */
void fs_open(struct fs_node *node, int flags __attribute__((unused))) {
    if(node->open) node->open(node);
}

void fs_close(struct fs_node *node) {
    if(node->close) node->close(node);
}

struct dirent *fs_readdir(struct fs_node *node, off_t idx) {
    if(node->readdir && (node->flags & 0x7) == FS_FLAG_DIR)
        return node->readdir(node, idx);
    else return NULL;
}

struct fs_node *fs_finddir(struct fs_node *node, char *name) {
    if(node->finddir && (node->flags & 0x7) == FS_FLAG_DIR)
        return node->finddir(node, name);
    else return NULL;
}

/* file contents is node->name */
static ssize_t test_read(struct fs_node *node, off_t off, size_t sz, uint8_t *buf) {
    size_t len = strnlen(node->name, sizeof node->name);

    if(off > len)
        return 0;
    if(off + sz > len)
        sz = len - off;

    memcpy(buf, node->name + off, sz);
    return sz;
}

void fs_init_test(void) {
    char contents[] = "this is a virtual file";

    fs_root = kmalloc(sizeof(struct fs_node));

    memcpy(fs_root->name, contents, sizeof contents);
    fs_root->read = test_read;
    fs_root->write = NULL;
    fs_root->open = NULL;
    fs_root->close = NULL;
    fs_root->readdir = NULL;
    fs_root->finddir = NULL;
}