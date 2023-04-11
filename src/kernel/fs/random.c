#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <features.h>
#include <string.h>

static uint32_t rand(void) {
    static uint32_t x = 123456789,
                    y = 262436069,
                    z = 521288629,
                    w = 88675123;

    uint32_t t;

    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

static ssize_t random_read(fs_node_t *node __unused, off_t off __unused, size_t sz, uint8_t *buf) {
    for(size_t s = 0; s < sz; s++)
        buf[s] = rand() % 0xff;
    return sz;
}

static fs_node_t *random_dev_create(void) {
    fs_node_t *node = kmalloc(sizeof(fs_node_t));
    memset(node, 0, sizeof(fs_node_t));
    node->inode = 0;
    strcpy(node->name, "random");
    node->uid = 0;
    node->gid = 0;
    node->mask = 0444;
    node->sz = 1024;
    node->flags = FS_FLAG_IFCHR;
    node->read = random_read;
    return node;
}

void random_install(void) {
    vfs_mount("/dev/random", random_dev_create());
}
