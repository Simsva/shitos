/**
 * @brief DOS/MBR partition parser for block devices
 */
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SECTORSZ 512

typedef struct partition {
    uint8_t active;
    uint8_t start_h, start_s, start_c;
    uint8_t type;
    uint8_t end_h, end_s, end_c;
    uint32_t start_lba;
    uint32_t num_sectors;
} partition_t;

typedef struct mbr {
    uint8_t bootstrap[440], uuid[4], reserved[2];
    partition_t part[4];
    uint8_t magic[2];
} __attribute__((packed)) mbr_t;

typedef struct dospart_device {
    partition_t part;
    fs_node_t *dev;
} dospart_device_t;

static ssize_t dospart_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    dospart_device_t *dev = node->device;

    if((size_t)off > dev->part.num_sectors * SECTORSZ) return 0;
    if((size_t)off + sz > dev->part.num_sectors * SECTORSZ)
        sz = dev->part.num_sectors * SECTORSZ - (size_t)off;

    off += dev->part.start_lba * SECTORSZ;
    return dev->dev->read(dev->dev, off, sz, buf);
}

static ssize_t dospart_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    dospart_device_t *dev = node->device;

    if((size_t)off > dev->part.num_sectors * SECTORSZ) return 0;
    if((size_t)off + sz > dev->part.num_sectors * SECTORSZ)
        sz = dev->part.num_sectors * SECTORSZ - (size_t)off;

    off += dev->part.start_lba * SECTORSZ;
    return dev->dev->write(dev->dev, off, sz, buf);
}

static fs_node_t *dospart_device_create(uint8_t i, fs_node_t *dev, mbr_t *mbr) {
    dospart_device_t *ddev = kmalloc(sizeof(dospart_device_t));
    memcpy(&ddev->part, &mbr->part[i], sizeof(partition_t));
    ddev->dev = dev;

    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    snprintf(fnode->name, sizeof fnode->name, "dospart%d", i);
    fnode->device = ddev;
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0660;
    fnode->sz = ddev->part.num_sectors * SECTORSZ;
    fnode->flags = FS_TYPE_BLOCK;

    fnode->read = dospart_read;
    fnode->write = dospart_write;

    return fnode;
}

static fs_node_t *dospart_map(const char *arg, __unused const char *mountpoint) {
    fs_node_t *dev = kopen(arg, 0);
    if(!dev) return NULL;

    mbr_t mbr;
    fs_read(dev, 0, sizeof mbr, (uint8_t *)&mbr);
    if(mbr.magic[0] != 0x55 || mbr.magic[1] != 0xaa) return NULL;

    for(uint8_t i = 0; i < 4; i++) {
        if(!(mbr.part[i].active & 0x80)) continue;

        fs_node_t *fnode = dospart_device_create(i, dev, &mbr);
        char devname[64];
        snprintf(devname, sizeof devname, "%s%d", arg, i);
        vfs_mount(devname, fnode);
    }

    return (fs_node_t *)1;
}

void dospart_init(void) {
    vfs_register_type("dospart", dospart_map);
}
