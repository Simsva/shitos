/**
 * @brief ShitOS native bootloader partition VFS driver.
 */
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <kernel/strsplit.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#define BOOTPART_HDR_MAGIC 0xb007beef

#define FLAG_VERBOSE 0x1

#define DPRINTF(lvl, fmt, ...) if(fs->flags & FLAG_VERBOSE) { \
    printf("bootpart [%s]: " fmt, #lvl __VA_OPT__(,) __VA_ARGS__); }

typedef struct bootpart_hdr {
    uint8_t code[3];
    uint8_t pad;
    uint32_t magic;
} bootpart_hdr_t;

typedef struct bootpart_fs {
    bootpart_hdr_t hdr;
    unsigned flags;
    fs_node_t *blockdev;
} bootpart_fs_t;

static fs_node_t *fstage1, *fstage2;

static ssize_t bootpart_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    bootpart_fs_t *fs = node->device;

    if((size_t)off > node->sz) return 0;
    if((size_t)off + sz > node->sz) sz = node->sz - (size_t)off;

    if(node->inode == 1)
        off += 512;
    return fs->blockdev->read(fs->blockdev, off, sz, buf);
}

static struct dirent *bootpart_readdir(__unused fs_node_t *node, off_t idx) {
    struct dirent *dirent = NULL;

    switch(idx) {
    case 0:
        dirent = kmalloc(sizeof(struct dirent));
        dirent->d_ino = 0;
        strcpy(dirent->d_name, "stage1.bin");
        break;
    case 1:
        dirent->d_ino = 1;
        strcpy(dirent->d_name, "stage2.bin");
        dirent = kmalloc(sizeof(struct dirent));
        break;
    }

    return dirent;
}

static fs_node_t *bootpart_finddir(__unused fs_node_t *node, const char *name) {
    if(!strcmp(name, "stage1.bin"))
        return fstage1;
    else if(!strcmp(name, "stage2.bin"))
        return fstage2;
    return NULL;
}

static fs_node_t *bootpart_mount(const char *arg, const char *mountpoint) {
    char *args = strdup(arg);
    char *argv[10];
    int argc = strsplit(args, ",", argv);

    fs_node_t *dev = kopen(argv[0], 0);
    if(!dev) {
        kfree(args);
        return NULL;
    }

    unsigned flags = 0;
    for(uint8_t i = 0; i < argc; i++) {
        if(!strcmp(argv[i], "verbose"))
            flags |= FLAG_VERBOSE;
    }
    kfree(args);

    bootpart_fs_t *fs = kmalloc(sizeof(bootpart_fs_t));
    memset(fs, 0, sizeof(bootpart_fs_t));
    fs_read(dev, 0, sizeof(bootpart_hdr_t), (uint8_t *)&fs->hdr);
    fs->flags = flags;
    fs->blockdev = dev;

    DPRINTF(INFO, "mounting boot partition at %s\n", mountpoint);

    if(fs->hdr.magic != BOOTPART_HDR_MAGIC) {
        DPRINTF(ERROR, "not a ShitOS boot partition\n");
        kfree(fs);
        return NULL;
    }

    /* create root node */
    fs_node_t *root = kmalloc(sizeof(fs_node_t));
    memset(root, 0, sizeof(fs_node_t));
    root->device = fs;
    strcpy(root->name, "boot");
    root->mask = 0755;
    root->flags = FS_FLAG_IFDIR | FS_FLAG_MOUNT;

    root->readdir = bootpart_readdir;
    root->finddir = bootpart_finddir;

    /* create stage nodes */
    fstage1 = kmalloc(sizeof(fs_node_t));
    memset(fstage1, 0, sizeof(fs_node_t));
    fstage1->device = fs;
    strcpy(fstage1->name, "stage1.bin");
    fstage1->sz = 512;
    fstage1->inode = 0;
    fstage1->refcount = -1;
    fstage1->read = bootpart_read;

    fstage2 = kmalloc(sizeof(fs_node_t));
    memset(fstage2, 0, sizeof(fs_node_t));
    fstage2->device = fs;
    strcpy(fstage2->name, "stage2.bin");
    fstage2->sz = dev->sz - 512;
    fstage2->inode = 1;
    fstage2->refcount = -1;
    fstage2->read = bootpart_read;

    return root;
}

void bootpart_init(void) {
    vfs_register_type("bootpart", bootpart_mount);
}
