/**
 * @brief EXT2 filesystem driver
 */
#include <ext2fs/ext2.h>
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <kernel/strsplit.h>
#include <kernel/args.h>
#include <stdio.h>
#include <string.h>

#define DPRINTF(lvl, fmt, ...)if(fs->flags & EXT2_FLAG_VERBOSE) { \
    printf("ext2fs [%s]: " fmt, #lvl __VA_OPT__(,) __VA_ARGS__); }

#define E_SUCCESS  0
#define E_BADBLOCK 1

static int read_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf);
static __unused int write_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf);
static ext2_inode_t *read_inode(ext2_fs_t *fs, uint32_t ino);
static int ext2fs_root(ext2_fs_t *fs, ext2_inode_t *ino, fs_node_t *fnode);
static fs_node_t *ext2fs_mount(const char *arg, const char *mountpoint);

/**
 * read_block reads an EXT2 block from the filesystem.
 */
static int read_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf) {
    /* 0 is an invalid block number */
    if(off == 0) return E_BADBLOCK;

    fs_read(fs->dev, off * fs->block_sz, fs->block_sz, buf);
    return E_SUCCESS;
}

/**
 * write_block writes an EXT2 block to the filesystem.
 */
static int write_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf) {
    /* 0 is an invalid block number */
    if(off == 0) return E_BADBLOCK;

    fs_write(fs->dev, off * fs->block_sz, fs->block_sz, buf);
    return E_SUCCESS;
}

/**
 * read_inode reads an inode from the filesystem and returns it.
 * Freed by the caller.
 */
static ext2_inode_t *read_inode(ext2_fs_t *fs, uint32_t ino) {
    uint32_t bg, bgi;
    ext2_inode_t *out, *inodes;
    /* inodes start at 1 */
    if(ino-- == 0) {
        DPRINTF(ERROR, "attempting to read inode 0\n");
        return NULL;
    }

    out = kmalloc(fs->inode_sz);

    bg  = ino / fs->inodes_per_group;
    bgi = ino % fs->inodes_per_group;

    inodes = kmalloc(fs->block_sz);
    read_block(fs, fs->bgdt[bg].bg_inode_table, (void *)inodes);
    memcpy(out, (void *)inodes + bgi*fs->inode_sz, fs->inode_sz);
    kfree(inodes);

    return out;
}

/**
 * Creates a root filesystem node from an inode.
 * Returns non-zero on error.
 */
static int ext2fs_root(ext2_fs_t *fs, ext2_inode_t *ino, fs_node_t *fnode) {
    if(!fnode || !ino) return 1;

    memset(fnode, 0, sizeof(fs_node_t));
    fnode->device = fs;
    fnode->inode = EXT2_ROOT_INO;
    strcpy(fnode->name, "/");
    fnode->uid = ino->i_uid;
    fnode->gid = ino->i_gid;
    fnode->sz = ino->i_size;
    fnode->mask = ino->i_mode & 07777;
    fnode->links_count = ino->i_links_count;

    /* flags */
    if(ino->i_mode & EXT2_TYPE_REG) {
        DPRINTF(CRITICAL, "root inode is a regular file!");
        return 1;
    }
    if(!(ino->i_mode & EXT2_TYPE_DIR)) {
        DPRINTF(CRITICAL, "root inode is not a directory!");
        return 1;
    }
    /* our VFS does not support multiple filetypes, so we ignore the rest */
    /* TODO: amend this */
    fnode->flags = FS_TYPE_DIR;

    /* TODO: file operations */
    return 0;
}

/**
 * ext2fs VFS mount callback
 */
static fs_node_t *ext2fs_mount(const char *arg, const char *mountpoint) {
    char *args = strdup(arg);
    char *argv[10];
    int argc = strsplit(args, ",", argv);

    fs_node_t *dev = kopen(argv[0], 0);
    if(!dev) {
        kfree(args);
        return NULL;
    }

    unsigned flags = 0;
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "rw"))
            flags |= EXT2_FLAG_RW;
        else if(!strcmp(argv[i], "verbose"))
            flags |= EXT2_FLAG_VERBOSE;
    }
    kfree(args);

    /* create the filesystem object */
    ext2_fs_t *fs = kmalloc(sizeof(ext2_fs_t));
    memset(fs, 0, sizeof(ext2_fs_t));
    fs->flags = flags;
    fs->dev = dev;

    DPRINTF(INFO, "mounting at %s with args %s\n", mountpoint, arg);

    /* load superblock */
    ext2_superblock_t *sb = kmalloc(sizeof(ext2_superblock_t));
    fs->sb = sb;
    /* temporary block size to read superblock correctly */
    fs->block_sz = 1024;
    read_block(fs, 1, (uint8_t *)sb);
    if(sb->s_magic != EXT2_MAGIC) {
        DPRINTF(ERROR, "not an EXT2 filesystem\n");
        kfree(sb);
        kfree(fs);
        fs_close(dev);
        return NULL;
    }
    fs->inode_sz = EXT2_INODE_SIZE(sb);
    fs->block_sz = EXT2_BLOCK_SIZE(sb);
    fs->inodes_per_group = sb->s_inodes_per_group;

    /* load the bgdt */
    fs->bgdt_sz = (sb->s_blocks_count + sb->s_blocks_per_group-1)
                / sb->s_blocks_per_group;
    fs->bgdt_block_sz = (fs->bgdt_sz * sizeof(ext2_group_desc_t) + fs->block_sz-1)
                      / fs->block_sz;
    fs->bgdt_off = (fs->block_sz > 1024) ? 1 : 2;

    ext2_group_desc_t *bgdt = kmalloc(fs->bgdt_block_sz * fs->block_sz);
    fs->bgdt = bgdt;
    for(uint8_t i = 0; i < fs->bgdt_block_sz; i++)
        read_block(fs, fs->bgdt_off + i, (void *)bgdt + i*fs->block_sz);

    DPRINTF(INFO, "bgdt_sz:%u inodes:%u ipg:%u\n",
            fs->bgdt_sz, sb->s_inodes_count, fs->inodes_per_group);

    /* read root inode */
    ext2_inode_t *root_ino = read_inode(fs, EXT2_ROOT_INO);
    fs->root_fnode = kmalloc(sizeof(fs_node_t));
    if(ext2fs_root(fs, root_ino, fs->root_fnode)) {
        DPRINTF(ERROR, "failed to create root node\n");
        kfree(sb);
        kfree(bgdt);
        kfree(fs->root_fnode);
        kfree(root_ino);
        kfree(fs);
        fs_close(dev);
        return NULL;
    }
    DPRINTF(INFO, "successfully mounted filesystem\n");
    kfree(root_ino);
    return fs->root_fnode;
}

void ext2fs_init(void) {
    vfs_register_type("ext2fs", ext2fs_mount);
}
