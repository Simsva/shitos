/**
 * @brief EXT2 filesystem driver
 *
 * TODO: free blocks, allocate/free inodes
 */
#include <ext2fs/ext2.h>
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <kernel/strsplit.h>
#include <kernel/args.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#define DPRINTF(lvl, fmt, ...) if(fs->flags & EXT2_FLAG_VERBOSE) { \
    printf("ext2fs [%s]: " fmt, #lvl __VA_OPT__(,) __VA_ARGS__); }

#define E_SUCCESS  0
#define E_BADBLOCK 1
#define E_NOSPACE  2

/* helper functions */
static int read_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf);
static int write_block(ext2_fs_t *fs, uint32_t off, uint8_t *buf);
static int write_superblock(ext2_fs_t *fs);
static int read_block_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t off, uint8_t *buf);
static int write_block_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t off, uint8_t *buf);
static ssize_t read_buffer_inode(ext2_fs_t *fs, ext2_inode_t *ino, off_t off, size_t sz, uint8_t *buf);
static ssize_t write_buffer_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, off_t off, size_t sz, uint8_t *buf);
static int refresh_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no);
static ext2_inode_t *read_inode(ext2_fs_t *fs, uint32_t ino);
static int write_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no);
static uint32_t alloc_block(ext2_fs_t *fs);
static int free_block(ext2_fs_t *fs, uint32_t block);
static uint32_t alloc_inode(ext2_fs_t *fs);
static int free_inode(ext2_fs_t *fs, uint32_t ino_no);
static int alloc_inode_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t block);
static int free_inode_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t block);
static int create_dirent(fs_node_t *parent, uint32_t ino_no, const char *name, uint8_t type);
static int create_dirent2(ext2_fs_t *fs, ext2_inode_t *pino, uint32_t pino_no, uint32_t ino_no, const char *name, uint8_t type);
static uint32_t get_real_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t block);
static int set_real_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t ino_block, uint32_t real_block);
static uint32_t get_block_count(ext2_fs_t *fs, uint32_t blocks);
static uint32_t get_content_blocks(ext2_fs_t *fs, uint32_t sectors);
static fs_node_t *fnode_from_dirent(ext2_fs_t *fs, ext2_inode_t *ino, ext2_dir_entry_t *dirent);

/* file operations */
static ssize_t ext2fs_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
static ssize_t ext2fs_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
static void ext2fs_open(fs_node_t *node, unsigned flags);
static void ext2fs_close(fs_node_t *node);
static struct dirent *ext2fs_readdir(fs_node_t *node, off_t idx);
static fs_node_t *ext2fs_finddir(fs_node_t *node, const char *name);
static ssize_t ext2fs_readlink(fs_node_t *node, char *buf, size_t sz);
static int ext2fs_truncate(fs_node_t *node, size_t sz);
static int ext2fs_mknod(fs_node_t *node, const char *name, mode_t mode);
static int ext2fs_unlink(fs_node_t *node, const char *name);
static int ext2fs_link(fs_node_t *node, const char *name, fs_node_t *target);
static int ext2fs_symlink(fs_node_t *node, const char *name, const char *target);
static int ext2fs_chmod(fs_node_t *node, mode_t mode);
static int ext2fs_chown(fs_node_t *node, uid_t uid, gid_t gid);

/* filesystem operations */
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
 * write_superblock rewrites the superblock to the filesystem. As the superblock
 * is always located on the same offset on disk (no matter the blocksize), this
 * requires special treatment.
 */
static int write_superblock(ext2_fs_t *fs) {
    fs_write(fs->dev, 1024, sizeof(ext2_superblock_t), (void *)fs->sb);
    return E_SUCCESS;
}

/**
 * read_block_inode reads the block at the specified offset in the inode.
 * Converts to real blocks.
 */
static int read_block_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t off, uint8_t *buf) {
    if(off >= get_content_blocks(fs, ino->i_blocks)) {
        /* NOTE: memset to 0 so writing out of bounds works correctly */
        memset(buf, 0, fs->block_sz);

        /* this warning will trigger everytime a block is allocated
         * TODO: better implementation */
        /* DPRINTF(WARN, "tried to read block %u in an inode with %u blocks (including indirect blocks)\n", */
        /*         off+1, ino->i_blocks / (fs->block_sz / 512)); */
        return E_BADBLOCK;
    }

    return read_block(fs, get_real_block(fs, ino, off), buf);
}

/**
 * write_block_inode writes the block at the specified offset in the inode.
 * Converts to real blocks.
 */
static int write_block_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t off, uint8_t *buf) {
    /* allocate blocks if they do not exist */
    uint32_t b;
    int ret = 0;
    while(off >= (b = get_content_blocks(fs, ino->i_blocks)))
        if((ret = alloc_inode_block(fs, ino, ino_no, b)) != E_SUCCESS)
            return ret;

    return write_block(fs, get_real_block(fs, ino, off), buf);
}

/**
 * read_buffer_inode reads the contents of an inode into a buffer. It works
 * pretty much exactly like ata_read in the IDE driver.
 */
static ssize_t read_buffer_inode(ext2_fs_t *fs, ext2_inode_t *ino, off_t off, size_t sz, uint8_t *buf) {
    /* TODO: error handling */
    if((size_t)off > ino->i_size) return 0;
    if((size_t)off + sz > ino->i_size)
        sz = ino->i_size - off;

    uint32_t start_block = off / fs->block_sz,
             end_block = (off + sz - 1) / fs->block_sz;
    size_t buf_offset = 0;

    /* if start is not on a block boundary
     * or if total size is less than one block */
    if(off % fs->block_sz || sz < fs->block_sz) {
        size_t prefix_sz = fs->block_sz - (off % fs->block_sz);
        if(prefix_sz > sz) prefix_sz = sz;

        uint8_t *tmp = kmalloc(fs->block_sz);
        read_block_inode(fs, ino, start_block, tmp);

        memcpy(buf, tmp + (off % fs->block_sz), prefix_sz);

        kfree(tmp);
        buf_offset += prefix_sz;
        /* first block is read */
        start_block++;
    }

    /* if end is not on a block boundary (and there are actually blocks left to
     * read after the first case)
     * if the total size fits within the start block, then do not run this even
     * though the end is not on a boundary */
    if((off + sz) % fs->block_sz && start_block <= end_block) {
        size_t postfix_sz = (off + sz) % fs->block_sz;
        uint8_t *tmp = kmalloc(fs->block_sz);
        read_block_inode(fs, ino, end_block, tmp);

        memcpy(buf + sz - postfix_sz, tmp, postfix_sz);

        kfree(tmp);
        /* last block is read */
        end_block--;
    }

    /* read the remaining blocks */
    while(start_block <= end_block) {
        read_block_inode(fs, ino, start_block, buf + buf_offset);
        buf_offset += fs->block_sz;
        start_block++;
    }

    return sz;
}

/**
 * write_buffer_inode writes a buffer into an inode. It works pretty much
 * exactly like ata_write in the IDE driver.
 */
static ssize_t write_buffer_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, off_t off, size_t sz, uint8_t *buf) {
    /* TODO: error handling, especially OOM/NOSPACE */

    /* resize inode to fit the buffer */
    if((size_t)off + sz > ino->i_size) {
        ino->i_size = off + sz;
        write_inode(fs, ino, ino_no);
    }

    uint32_t start_block = off / fs->block_sz,
             end_block = (off + sz - 1) / fs->block_sz;
    size_t buf_offset = 0;

    /* if start is not on a block boundary
     * or if total size is less than one block */
    if(off % fs->block_sz || sz < fs->block_sz) {
        size_t prefix_sz = fs->block_sz - (off % fs->block_sz);
        if(prefix_sz > sz) prefix_sz = sz;

        uint8_t *tmp = kmalloc(fs->block_sz);
        read_block_inode(fs, ino, start_block, tmp);

        memcpy(tmp + (off % fs->block_sz), buf, prefix_sz);
        write_block_inode(fs, ino, ino_no, start_block, tmp);

        kfree(tmp);
        buf_offset += prefix_sz;
        /* first block is written */
        start_block++;
    }

    /* if end is not on a block boundary */
    if((off + sz) % fs->block_sz && start_block <= end_block) {
        size_t postfix_sz = (off + sz) % fs->block_sz;
        uint8_t *tmp = kmalloc(fs->block_sz);
        read_block_inode(fs, ino, end_block, tmp);

        memcpy(tmp, buf + sz - postfix_sz, postfix_sz);
        write_block_inode(fs, ino, ino_no, end_block, tmp);

        kfree(tmp);
        /* last block is written */
        end_block--;
    }

    /* write the remaining blocks */
    while(start_block <= end_block) {
        write_block_inode(fs, ino, ino_no, start_block, buf + buf_offset);
        buf_offset += fs->block_sz;
        start_block++;
    }

    return sz;
}

/**
 * refresh_inode reads an inode from the filesystem into ino.
 */
static int refresh_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no) {
    uint32_t bg, bgi, bgoff;
    ext2_inode_t *inodes;
    /* inodes start at 1 */
    if(ino_no-- == 0) {
        DPRINTF(ERROR, "attempting to read inode 0\n");
        return E_BADBLOCK;
    }

    bg    = ino_no / fs->inodes_per_group;
    bgi   = ino_no % fs->inodes_per_group;
    bgoff = (ino_no * fs->inode_sz) / fs->block_sz;
    bgi   = (bgi * fs->inode_sz) % fs->block_sz;

    if(bg > fs->bgdt_sz) {
        DPRINTF(ERROR, "attempting to read an inode outside the BGDT\n");
        return E_BADBLOCK;
    }

    inodes = kmalloc(fs->block_sz);
    read_block(fs, fs->bgdt[bg].bg_inode_table + bgoff, (void *)inodes);
    memcpy(ino, (void *)inodes + bgi, fs->inode_sz);
    kfree(inodes);

    return E_SUCCESS;
}

/**
 * read_inode reads an inode from the filesystem and returns it.
 * Freed by the caller.
 */
static ext2_inode_t *read_inode(ext2_fs_t *fs, uint32_t ino) {
    ext2_inode_t *out;
    out = kmalloc(fs->inode_sz);
    if(refresh_inode(fs, out, ino) == E_SUCCESS)
        return out;

    kfree(out);
    return NULL;
}

/**
 * write_inode writes an inode to the filesystem at the specified inode number.
 */
static int write_inode(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no) {
    uint32_t bg, bgi, bgoff;
    ext2_inode_t *inodes;
    /* inodes start at 1 */
    if(ino_no-- == 0) {
        DPRINTF(ERROR, "attempting to write inode 0\n");
        return E_BADBLOCK;
    }

    bg    = ino_no / fs->inodes_per_group;
    bgi   = ino_no % fs->inodes_per_group;
    bgoff = (ino_no * fs->inode_sz) / fs->block_sz;
    bgi   = (bgi * fs->inode_sz) % fs->block_sz;

    if(bg > fs->bgdt_sz) {
        DPRINTF(ERROR, "attempting to write an inode outside the BGDT\n");
        return E_BADBLOCK;
    }

    inodes = kmalloc(fs->block_sz);
    read_block(fs, fs->bgdt[bg].bg_inode_table + bgoff, (void *)inodes);
    memcpy((void *)inodes + bgi, ino, fs->inode_sz);
    write_block(fs, fs->bgdt[bg].bg_inode_table + bgoff, (void *)inodes);
    kfree(inodes);

    return E_SUCCESS;
}

/**
 * alloc_block will allocate a free EXT2 block on disk, zero it out, and return
 * its index.
 * Returns zero on error.
 */
static uint32_t alloc_block(ext2_fs_t *fs) {
    uint32_t blockno, blocki, blockoff, bg;
    uint32_t *buf = kmalloc(fs->block_sz);

    for(bg = 0, blockno = 0; bg < fs->bgdt_sz; bg++) {
        if(fs->bgdt[bg].bg_free_blocks_count == 0) continue;

        read_block(fs, fs->bgdt[bg].bg_block_bitmap, (uint8_t *)buf);
        for(blocki = 0; blocki < (fs->sb->s_blocks_per_group>>3) / sizeof(uint32_t); blocki++) {
            /* the bitmap SHOULD mark block 0 as used */
            if((blockoff = ffsl(~buf[blocki]))) {
                /* found bit offset + block index + block group index */
                blockno = blockoff-1 + 32*blocki + bg*fs->sb->s_blocks_per_group;
                goto found;
            }
        }
    }
found:
    if(!blockno) {
        DPRINTF(CRITICAL, "no free blocks left\n");
        kfree(buf);
        return 0;
    }

    buf[blocki] |= 1 << (blockoff-1);
    write_block(fs, fs->bgdt[bg].bg_block_bitmap, (uint8_t *)buf);

    /* update the BGDT */
    fs->bgdt[bg].bg_free_blocks_count--;
    for(uint32_t i = 0; i < fs->bgdt_block_sz; i++)
        write_block(fs, fs->bgdt_off + i, (void *)fs->bgdt + fs->block_sz*i);

    /* update the superblock */
    fs->sb->s_free_blocks_count--;
    write_superblock(fs);

    /* zero out found block */
    memset(buf, 0, fs->block_sz);
    write_block(fs, blockno, (uint8_t *)buf);

    kfree(buf);
    return blockno;
}

/**
 * free_block takes an inode number and frees it.
 */
static int free_block(ext2_fs_t *fs, uint32_t block) {
    uint32_t bg, bgi, blockoff, blocki, *bitmap;

    if(block == 0) {
        DPRINTF(ERROR, "trying to free block 0\n");
        return E_BADBLOCK;
    }

    bg  = block / fs->sb->s_blocks_per_group;
    bgi = block % fs->sb->s_blocks_per_group;
    blockoff = bgi / 32;
    blocki   = bgi % 32;

    bitmap = kmalloc(fs->block_sz);
    read_block(fs, fs->bgdt[bg].bg_block_bitmap, (void *)bitmap);
    bitmap[blockoff] |= 1<<blocki;
    write_block(fs, fs->bgdt[bg].bg_block_bitmap, (void *)bitmap);
    kfree(bitmap);

    return E_SUCCESS;
}

/**
 * alloc_inode will allocate a free inode and return it.
 * Returns zero on error.
 */
static uint32_t alloc_inode(ext2_fs_t *fs) {
    uint32_t inono, blocki, blockoff, bg;
    uint32_t *buf = kmalloc(fs->block_sz);

    for(bg = 0, inono = 0; bg < fs->bgdt_sz; bg++) {
        if(fs->bgdt[bg].bg_free_inodes_count == 0) continue;

        read_block(fs, fs->bgdt[bg].bg_inode_bitmap, (uint8_t *)buf);
        for(blocki = 0; blocki < (fs->sb->s_inodes_per_group>>3) / sizeof(uint32_t); blocki++) {
            /* the bitmap SHOULD mark the reserved inodes as used */
            if((blockoff = ffsl(~buf[blocki]))) {
                /* found bit offset (+1 because inode) + block index + block group index */
                inono = blockoff + 32*blocki + bg*fs->sb->s_inodes_per_group;
                goto found;
            }
        }
    }
found:
    if(!inono) {
        DPRINTF(CRITICAL, "no free inodes left\n");
        kfree(buf);
        return 0;
    }

    buf[blocki] |= 1 << (blockoff-1);
    write_block(fs, fs->bgdt[bg].bg_inode_bitmap, (uint8_t *)buf);

    /* update the BGDT */
    fs->bgdt[bg].bg_free_inodes_count--;
    for(uint32_t i = 0; i < fs->bgdt_block_sz; i++)
        write_block(fs, fs->bgdt_off + i, (void *)fs->bgdt + fs->block_sz*i);

    /* update the superblock */
    fs->sb->s_free_inodes_count--;
    write_superblock(fs);

    kfree(buf);
    return inono;
}

/**
 * free_inode takes an inode number and frees it.
 */
static int free_inode(ext2_fs_t *fs, uint32_t ino_no) {
    uint32_t bg, bgi, blockoff, blocki, *bitmap;

    if(ino_no-- == 0) {
        DPRINTF(ERROR, "trying to free inode 0\n");
        return E_BADBLOCK;
    }

    bg  = ino_no / fs->sb->s_inodes_per_group;
    bgi = ino_no % fs->sb->s_inodes_per_group;
    blockoff = bgi / 32;
    blocki   = bgi % 32;

    bitmap = kmalloc(fs->block_sz);
    read_block(fs, fs->bgdt[bg].bg_inode_bitmap, (void *)bitmap);
    bitmap[blockoff] |= 1<<blocki;
    write_block(fs, fs->bgdt[bg].bg_inode_bitmap, (void *)bitmap);
    kfree(bitmap);

    return E_SUCCESS;
}

/**
 * alloc_inode_block allocates a block at the specified offset in the inode.
 *
 * BUG: if an indirect block doesn't exist, it will allocate the first "content"
 * block first, thus making consecutively allocated "content" blocks
 * noncontiguous even if it would be possible for them to be contiguous.
 */
static int alloc_inode_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t block) {
    uint32_t real_block = alloc_block(fs);
    if(!real_block) return E_NOSPACE;

    uint32_t err;
    if((err = set_real_block(fs, ino, ino_no, block, real_block)) != E_SUCCESS)
        return err;

    uint32_t sectors = get_block_count(fs, block+1);
    if(ino->i_blocks < sectors) ino->i_blocks = sectors;
    return write_inode(fs, ino, ino_no);
}

static int free_inode_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t block);

/**
 * create_dirent creates a directory entry in parent to the specified inode.
 *
 * Returns zero or a negative errno.
 */
static int create_dirent(fs_node_t *parent, uint32_t ino_no, const char *name, uint8_t type) {
    ext2_fs_t *fs = parent->device;
    ext2_inode_t *pino = read_inode(fs, parent->inode);
    /* we trust the fs_* functions to have checked that
     * the parent and name are correct */

    int ret = create_dirent2(fs, pino, parent->inode, ino_no, name, type);
    kfree(pino);
    return ret;
}

/**
 * create_dirent2 is the same as create_dirent, but takes different arguments.
 */
static int create_dirent2(ext2_fs_t *fs, ext2_inode_t *pino, uint32_t pino_no, uint32_t ino_no, const char *name, uint8_t type) {
    size_t name_len = strlen(name);
    /* rec_len has to be divisible by 4 */
    size_t rec_len = (sizeof(ext2_dir_entry_t) + name_len + 3) & ~3;

    uint32_t blockno = 0, offset = 0;
    uint8_t *buf = kmalloc(fs->block_sz);
    read_block_inode(fs, pino, blockno, buf);
    ext2_dir_entry_t *dirent = (void *)buf;
    int found = 0, ret = 0;

    while(offset < pino->i_size) {
        if(!dirent->inode && dirent->rec_len >= rec_len) {
            /* our entry fits in this empty entry, replace it */
            found = 1;
            break;
        }

        /* if the record spans to the end of the block,
         * we can maybe fit our direntry in the same block? */
        uint16_t real_rec_len = (sizeof(ext2_dir_entry_t) + dirent->name_len + 3) & ~3;
        if(real_rec_len != dirent->rec_len &&
           (fs->block_sz - (real_rec_len + offset % fs->block_sz)) >= rec_len) {
            dirent->rec_len = real_rec_len;

            offset += real_rec_len;
            dirent = (void *)dirent + real_rec_len;

            dirent->rec_len = fs->block_sz - offset;
            found = 1;
            break;
        }

        offset += dirent->rec_len;
        dirent = (void *)dirent + dirent->rec_len;

        if((uintptr_t)dirent - (uintptr_t)buf >= fs->block_sz) {
            dirent = (void *)dirent - fs->block_sz;
            /* will error on an OOB read, which we rely on so
             * we can't use this for error handling reliably */
            read_block_inode(fs, pino, ++blockno, buf);
        }
    }

    if(!found) {
        /* we exceeded the node size, which means buf is an empty block */
        dirent->rec_len = fs->block_sz;
        pino->i_size += fs->block_sz;
        write_inode(fs, pino, pino_no);
    }

    dirent->inode = ino_no;
    dirent->name_len = name_len;
    dirent->type = type;
    memcpy(dirent->name, name, name_len);

    /* TODO: don't assume every error is ENOSPC */
    ret = write_block_inode(fs, pino, pino_no, blockno, buf)
        ? -ENOSPC
        : 0;

    kfree(buf);
    return ret;
}

/**
 * Returns the real block number of a specified block in an inode. I.e. it takes
 * into account indirect blocks.
 */
static uint32_t get_real_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t off) {
    /* pointers per block */
    uint32_t p = fs->block_sz / sizeof(uint32_t);

    /* indirect block storage */
    uint32_t *tmp;
    /* indirect block math */
    uint32_t a, idx1, idx2, idx3;
    uint32_t intermediate;

    if(off < EXT2_NDIR_BLOCKS) {
        /* direct block */
        return ino->i_block[off];
    } else if(off < EXT2_NDIR_BLOCKS + p) {
        /* singly indirect block */
        if(!ino->i_block[EXT2_SIND_BLOCK]) {
            DPRINTF(ERROR, "trying to read a non-existant SIND block\n");
            return 0;
        }

        idx1 = off - EXT2_NDIR_BLOCKS;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_SIND_BLOCK], (uint8_t *)tmp);
        intermediate = tmp[idx1];

        kfree(tmp);
        return intermediate;
    } else if(off < EXT2_NDIR_BLOCKS + p + p*p) {
        /* doubly indirect block */
        if(!ino->i_block[EXT2_DIND_BLOCK]) {
            DPRINTF(ERROR, "trying to read a non-existant DIND block\n");
            return 0;
        }

        a = off - EXT2_NDIR_BLOCKS - p;
        idx1 = a / p;
        idx2 = a % p;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_DIND_BLOCK], (uint8_t *)tmp);
        intermediate = tmp[idx1];
        if(!intermediate) {
            DPRINTF(ERROR, "trying to read a non-existant DIND block\n");
            kfree(tmp);
            return 0;
        }

        read_block(fs, intermediate, (uint8_t *)tmp);
        intermediate = tmp[idx2];

        kfree(tmp);
        return intermediate;
    } else { /*if(off < EXT2_NDIR_BLOCKS + p + p*p + p*p*p) {*/
        /* triply indirect block */
        /* XXX: this also happens on too high block numbers */

        if(!ino->i_block[EXT2_TIND_BLOCK]) {
            DPRINTF(ERROR, "trying to read a non-existant TIND block\n");
            return 0;
        }

        a = off - EXT2_NDIR_BLOCKS - p - p*p;
        idx1 = a / (p*p);
        a = a % (p*p);
        idx2 = a / p;
        idx3 = a % p;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_TIND_BLOCK], (uint8_t *)tmp);
        intermediate = tmp[idx1];
        if(!intermediate) {
            DPRINTF(ERROR, "trying to read a non-existant TIND block\n");
            kfree(tmp);
            return 0;
        }

        read_block(fs, intermediate, (uint8_t *)tmp);
        intermediate = tmp[idx2];
        if(!intermediate) {
            DPRINTF(ERROR, "trying to read a non-existant TIND block\n");
            kfree(tmp);
            return 0;
        }

        read_block(fs, intermediate, (uint8_t *)tmp);
        intermediate = tmp[idx3];

        kfree(tmp);
        return intermediate;
    }

    /* XXX: unreachable, TODO: add OOB check */
    DPRINTF(ERROR, "trying to read a block number higher than the maximum\n");
    return 0;
}

/**
 * set_real_block assigns real_block to the block at offset ino_block in an inode.
 */
static int set_real_block(ext2_fs_t *fs, ext2_inode_t *ino, uint32_t ino_no, uint32_t ino_block, uint32_t real_block) {
    /* pointers per block */
    uint32_t p = fs->block_sz / sizeof(uint32_t);

    /* indirect block storage */
    uint32_t *tmp;
    /* indirect block math */
    uint32_t a, idx1, idx2, idx3;
    /* intermediate block */
    uint32_t intermediate;
    int ret;

    if(ino_block < EXT2_NDIR_BLOCKS) {
        /* direct block */
        ino->i_block[ino_block] = real_block;
        return write_inode(fs, ino, ino_no);
    } else if(ino_block < EXT2_NDIR_BLOCKS + p) {
        /* singly indirect block */
        if(!ino->i_block[EXT2_SIND_BLOCK]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block) return E_NOSPACE;
            ino->i_block[EXT2_SIND_BLOCK] = new_block;
            write_inode(fs, ino, ino_no);
        }

        idx1 = ino_block - EXT2_NDIR_BLOCKS;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_SIND_BLOCK], (uint8_t *)tmp);
        tmp[idx1] = real_block;
        ret = write_block(fs, ino->i_block[EXT2_SIND_BLOCK], (uint8_t *)tmp);

        goto ret_free;
    } else if(ino_block < EXT2_NDIR_BLOCKS + p + p*p) {
        /* doubly indirect block */
        if(!ino->i_block[EXT2_DIND_BLOCK]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block) return E_NOSPACE;
            ino->i_block[EXT2_DIND_BLOCK] = new_block;
            write_inode(fs, ino, ino_no);
        }

        a = ino_block - EXT2_NDIR_BLOCKS - p;
        idx1 = a / p;
        idx2 = a % p;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_DIND_BLOCK], (uint8_t *)tmp);
        if(!tmp[idx1]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block){
                ret = E_NOSPACE;
                goto ret_free;
            }
            tmp[idx1] = new_block;
            /* write the DIND */
            if((ret = write_block(fs, ino->i_block[EXT2_DIND_BLOCK], (uint8_t *)tmp)) != E_SUCCESS)
                goto ret_free;
        }

        intermediate = tmp[idx1];
        read_block(fs, intermediate, (uint8_t *)tmp);
        tmp[idx2] = real_block;
        ret = write_block(fs, intermediate, (uint8_t *)tmp);

        goto ret_free;
    } else { /*if(ino_block < EXT2_NDIR_BLOCKS + p + p*p + p*p*p) {*/
        /* triply indirect block */
        /* XXX: this also happens on too high block numbers */

        if(!ino->i_block[EXT2_TIND_BLOCK]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block) return E_NOSPACE;
            ino->i_block[EXT2_TIND_BLOCK] = new_block;
            write_inode(fs, ino, ino_no);
        }

        a = ino_block - EXT2_NDIR_BLOCKS - p - p*p;
        idx1 = a / (p*p);
        a %= (p*p);
        idx2 = a / p;
        idx3 = a % p;

        tmp = kmalloc(fs->block_sz);
        read_block(fs, ino->i_block[EXT2_TIND_BLOCK], (uint8_t *)tmp);
        if(!tmp[idx1]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block){
                ret = E_NOSPACE;
                goto ret_free;
            }
            tmp[idx1] = new_block;
            /* write the TIND */
            if((ret = write_block(fs, ino->i_block[EXT2_TIND_BLOCK], (uint8_t *)tmp)) != E_SUCCESS)
                goto ret_free;
        }

        intermediate = tmp[idx1];
        read_block(fs, intermediate, (uint8_t *)tmp);
        if(!tmp[idx2]) {
            uint32_t new_block = alloc_block(fs);
            if(!new_block){
                ret = E_NOSPACE;
                goto ret_free;
            }
            tmp[idx2] = new_block;
            /* write the intermediate DIND */
            if((ret = write_block(fs, intermediate, (uint8_t *)tmp)) != E_SUCCESS)
                goto ret_free;
        }

        intermediate = tmp[idx2];
        read_block(fs, intermediate, (uint8_t *)tmp);
        tmp[idx3] = real_block;
        ret = write_block(fs, intermediate, (uint8_t *)tmp);

        goto ret_free;
    }

    /* XXX: unreachable, TODO: add OOB check */
    DPRINTF(ERROR, "trying to read a block number higher than the maximum\n");
    return E_BADBLOCK;
ret_free:
    kfree(tmp);
    return ret;
}

/**
 * get_block_count returns the amount of used disk sectors of an inode with the
 * specified amount of content blocks. Takes into account indirect blocks. In
 * other words it computes ext2_inode_t::i_blocks.
 */
static uint32_t get_block_count(ext2_fs_t *fs, uint32_t blocks) {
    /* pointers per block */
    uint32_t p = fs->block_sz / sizeof(uint32_t);
    uint32_t a, b, c, ind_blocks;

    if(blocks <= EXT2_NDIR_BLOCKS) {
        /* direct blocks only */
        ind_blocks = 0;
    } else if(blocks <= EXT2_NDIR_BLOCKS + p) {
        /* singly indirect block */
        ind_blocks = 1;
    } else if(blocks <= EXT2_NDIR_BLOCKS + p + p*p) {
        /* doubly indirect block */

        /* amount of blocks stored as doubly indirect */
        a = blocks - EXT2_NDIR_BLOCKS - p;

        /* max sind + dind + amount of sind in dind */
        ind_blocks = 1 + 1 + (a + p-1) / p;
    } else {
        /* triply indirect block, illegal math incoming */
        /* XXX: this also happens on too high block numbers */

        /* amount of blocks stored as triply indirect */
        a = blocks - EXT2_NDIR_BLOCKS - p - p*p;
        /* amount of dind blocks in the tind */
        b = (a + p*p-1) / (p*p);
        /* amount of blocks stored in the last dind */
        c = 1 + (a-1) % (p*p);

        /* sind + full first dind + tind + dind in tind + sind in full dinds
         * + sind in last dind */
        ind_blocks = 1 + 1+p + 1 + b + (b-1)*p + (c + p-1) / p;
    }

    /* convert to disk sectors */
    return (blocks + ind_blocks) * (fs->block_sz / 512);
}

/**
 * get_content_blocks returns the amount of blocks containing the actual file
 * contents of an inode taking up a specified amount of disk sectors.
 * Essentially the inverse of get_block_count.
 */
static uint32_t get_content_blocks(ext2_fs_t *fs, uint32_t sectors) {
    uint32_t blocks = sectors / (fs->block_sz / 512);
    /* pointers per block */
    uint32_t p = fs->block_sz / sizeof(uint32_t);
    uint32_t a, ind_blocks;

    /* this entire function is pure magic, don't ask */
    if(blocks <= EXT2_NDIR_BLOCKS) {
        /* direct blocks only */
        ind_blocks = 0;
    } else if(blocks <= EXT2_NDIR_BLOCKS + p + 1) {
        /* singly indirect block */
        ind_blocks = 1;
    } else if(blocks <= EXT2_NDIR_BLOCKS + p + p*p + 1 + 1 + p) {
        /* doubly indirect block */

        /* "index" in the blocks stored as doubly indirect
         * minus the dind block itself */
        a = blocks - EXT2_NDIR_BLOCKS - p - 1 - 1 - 1;

        /* sind + dind + at least one sind in the dind */
        ind_blocks = 1 + 1 + 1;

        /* add 1 to ind_blocks for every sind
         * one sind includes p blocks plus the sind itself */
        ind_blocks += a / (p+1);
    } else {
        /* triply indirect block */
        /* XXX: this also happens on too high block numbers */

        /* "index" in the blocks stored as triply indirect
         * minus the tind block itself */
        a = blocks - EXT2_NDIR_BLOCKS - p - p*p - 1 - 1 - p - 1 - 1;

        /* sind + full dind + tind + at least one dind in the tind
         * + at least one sind in the last dind */
        ind_blocks = 1 + 1+p + 1 + 1 + 1;

        /* add p + 1 to ind_blocks for every dind
         * one dind includes p*p blocks, p sind blocks and itself */
        ind_blocks += (p + 1) * (a / (p*p + p + 1));
        a %= p*p + p + 1;

        /* we now don't care about the dind, so remove it from a */
        a--;
        /* add 1 to ind for every sind in the dind,
         * same as the dind case above */
        ind_blocks += a / (p + 1);
    }

    /* return only the non-indirect blocks */
    return blocks - ind_blocks;
}

/**
 * Return a VFS node from a EXT2 directory entry and inode.
 * If ino is NULL, it will be read from dirent. The caller can provide it if it
 * already exists to avoid reading it twice.
 */
static fs_node_t *fnode_from_dirent(ext2_fs_t *fs, ext2_inode_t *ino, ext2_dir_entry_t *dirent) {
    if(!dirent->inode || dirent->inode == EXT2_BAD_INO) return NULL;
    if(!ino) ino = read_inode(fs, dirent->inode);

    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));

    /* dirent info */
    fnode->device = fs;
    fnode->inode = dirent->inode;
    memcpy(fnode->name, dirent->name, dirent->name_len);
    fnode->name[dirent->name_len] = '\0';

    /* inode info */
    fnode->mask = ino->i_mode & 07777;
    fnode->sz = ino->i_size;
    fnode->uid = ino->i_uid;
    fnode->gid = ino->i_gid;
    fnode->links_count = ino->i_links_count;

    /* flags */
    fnode->flags = 0;
    switch(ino->i_mode & EXT2_TYPE_MASK) {
    case EXT2_TYPE_REG:
        fnode->flags |= FS_FLAG_IFREG;
        fnode->read = ext2fs_read;
        fnode->write = ext2fs_write;
        fnode->truncate = ext2fs_truncate;
        break;

    case EXT2_TYPE_DIR:
        fnode->flags |= FS_FLAG_IFDIR;
        fnode->readdir = ext2fs_readdir;
        fnode->finddir = ext2fs_finddir;
        fnode->mknod = ext2fs_mknod;
        fnode->unlink = ext2fs_unlink;
        fnode->link = ext2fs_link;
        fnode->symlink = ext2fs_symlink;
        break;

    case EXT2_TYPE_BLOCK:
        fnode->flags |= FS_FLAG_IFBLK;
        break;

    case EXT2_TYPE_CHAR:
        fnode->flags |= FS_FLAG_IFCHR;
        break;

    case EXT2_TYPE_FIFO:
        fnode->flags |= FS_FLAG_IFIFO;
        break;

    case EXT2_TYPE_LINK:
        fnode->flags |= FS_FLAG_IFLNK;
        fnode->readlink = ext2fs_readlink;
        break;

    default:
        /* bad type */
        kfree(fnode);
        return NULL;
    }

    fnode->open = ext2fs_open;
    fnode->close = ext2fs_close;
    fnode->chmod = ext2fs_chmod;
    fnode->chown = ext2fs_chown;

    return fnode;
}

static ssize_t ext2fs_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    ssize_t out = read_buffer_inode(fs, ino, off, sz, buf);
    kfree(ino);
    return out;
}

static ssize_t ext2fs_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    ext2_fs_t *fs = node->device;
    if(!(fs->flags & EXT2_FLAG_RW)) return -EROFS;

    ext2_inode_t *ino = read_inode(fs, node->inode);

    ssize_t out = write_buffer_inode(fs, ino, node->inode, off, sz, buf);
    kfree(ino);
    return out;
}

static void ext2fs_open(__unused fs_node_t *node, __unused unsigned flags) {
    return;
}

static void ext2fs_close(__unused fs_node_t *node) {
    return;
}

static struct dirent *ext2fs_readdir(fs_node_t *node, off_t idx) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    uint32_t blockno = 0, offset = 0;
    off_t curidx = 0;
    uint8_t *buf = kmalloc(fs->block_sz);
    read_block_inode(fs, ino, blockno, buf);
    ext2_dir_entry_t *dirent = (void *)buf;

    while(offset < node->sz && curidx <= idx) {
        if(dirent->inode && curidx == idx) {
            struct dirent *out = kmalloc(sizeof(struct dirent));
            out->d_ino = dirent->inode;
            memcpy(out->d_name, dirent->name, dirent->name_len);
            out->d_name[dirent->name_len] = '\0';
            kfree(ino);
            kfree(buf);
            return out;
        }

        offset += dirent->rec_len;
        dirent = (void *)dirent + dirent->rec_len;
        if(dirent->inode) curidx++;

        if((uintptr_t)dirent - (uintptr_t)buf >= fs->block_sz) {
            dirent = (void *)dirent - fs->block_sz;
            read_block_inode(fs, ino, ++blockno, buf);
        }
    }
    kfree(ino);
    kfree(buf);
    return NULL;
}

static fs_node_t *ext2fs_finddir(fs_node_t *node, const char *name) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    char namebuf[256];
    uint32_t blockno = 0, offset = 0;
    uint8_t *buf = kmalloc(fs->block_sz);
    read_block_inode(fs, ino, blockno, buf);
    ext2_dir_entry_t *dirent = (void *)buf;
    int found = 0;
    size_t name_len = strlen(name);

    while(offset < node->sz) {
        /* only test real possibilities */
        if(dirent->inode && dirent->name_len == name_len) {
            /* NOTE: a memcmp could read OOB on the given name */
            memcpy(namebuf, dirent->name, dirent->name_len);
            namebuf[dirent->name_len] = '\0';
            if(!strcmp(namebuf, name)) {
                found = 1;
                break;
            }
        }

        offset += dirent->rec_len;
        dirent = (void *)dirent + dirent->rec_len;

        if((uintptr_t)dirent - (uintptr_t)buf >= fs->block_sz) {
            dirent = (void *)dirent - fs->block_sz;
            read_block_inode(fs, ino, ++blockno, buf);
        }
    }
    if(!found || dirent->inode == 0) {
        kfree(buf);
        kfree(ino);
        return NULL;
    }
    ino = read_inode(fs, dirent->inode);
    fs_node_t *fnode = fnode_from_dirent(fs, ino, dirent);

    kfree(ino);
    kfree(buf);
    return fnode ? fnode : NULL;
}

static ssize_t ext2fs_readlink(fs_node_t *node, char *buf, size_t sz) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    size_t read_sz = ino->i_size > sz ? sz : ino->i_size;

    /* if the link fits inside the space used for block pointers it will be
     * stored there instead of in a block */
    if(ino->i_size > EXT2_N_BLOCKS * sizeof(uint32_t))
        read_buffer_inode(fs, ino, 0, read_sz, (uint8_t *)buf);
    else
        memcpy(buf, (void *)ino->i_block, read_sz);

    /* sz does not cover the null byte */
    if(read_sz < sz) buf[read_sz] = '\0';

    kfree(ino);
    return read_sz;
}

static int ext2fs_truncate(fs_node_t *node, size_t sz) {
    return 0;
}

static int ext2fs_mknod(fs_node_t *node, const char *name, mode_t mode) {
    ext2_fs_t *fs = node->device;
    int ret = 0;

    /* allocate an inode */
    uint32_t newino_no = alloc_inode(fs);
    if(!newino_no) return -ENOSPC;
    ext2_inode_t *newino = kmalloc(fs->inode_sz);
    memset(newino, 0, fs->inode_sz);

    /* TODO: actual time */
    newino->i_atime = newino->i_ctime = newino->i_mtime = 0;
    newino->i_dtime = 0;

    /* TODO: users? */
    newino->i_uid = 0;
    newino->i_gid = 0;

    newino->i_links_count = 1;
    newino->i_mode = mode;
    /* all other fields are zero */

    /* create directory entry */
    if((ret = create_dirent(node, newino_no, name, EXT2_DIRENT_TYPE_LINK))) {
        /* free the new inode on failure */
        free_inode(fs, newino_no);
        goto ret_free;
    }

    /* if the new node is a directory we need to create . and .. */
    if((newino->i_mode & EXT2_TYPE_MASK) == EXT2_TYPE_DIR) {
        newino->i_links_count += 2;

        /* this will leave a mess if it errors... */
        if((ret = create_dirent2(fs, newino, newino_no, newino_no, ".", EXT2_DIRENT_TYPE_DIR)))
            goto ret_free;
        if((ret = create_dirent2(fs, newino, newino_no, node->inode, "..", EXT2_DIRENT_TYPE_DIR)))
            goto ret_free;
    }

    /* save the inode to disk */
    write_inode(fs, newino, newino_no);
    ret = 0;
ret_free:
    kfree(newino);
    return ret;
}

static int ext2fs_unlink(fs_node_t *node, const char *name) {
    return 0;
}

static int ext2fs_link(fs_node_t *node, const char *name, fs_node_t *target) {
    ext2_fs_t *fs = node->device;
    int ret;
    if(node->device != target->device) return -EXDEV;

    /* read target inode */
    ext2_inode_t *ino = read_inode(fs, target->inode);
    uint8_t dirent_type = EXT2_DIRENT_TYPE_UNK;
    switch(ino->i_mode & EXT2_TYPE_MASK) {
    case EXT2_TYPE_REG:
        dirent_type = EXT2_DIRENT_TYPE_REG;
        break;

    case EXT2_TYPE_DIR:
        /* cannot make a hardlink to a directory */
        ret = -EISDIR;
        goto ret_free;

    case EXT2_TYPE_CHAR:
        dirent_type = EXT2_DIRENT_TYPE_CHAR;
        break;

    case EXT2_TYPE_BLOCK:
        dirent_type = EXT2_DIRENT_TYPE_BLOCK;
        break;

    case EXT2_TYPE_FIFO:
        dirent_type = EXT2_DIRENT_TYPE_FIFO;
        break;

    case EXT2_TYPE_SOCK:
        dirent_type = EXT2_DIRENT_TYPE_SOCK;
        break;

    case EXT2_TYPE_LINK:
        dirent_type = EXT2_DIRENT_TYPE_LINK;
        break;
    }

    /* create the directory entry */
    if((ret = create_dirent(node, target->inode, name, dirent_type)))
        goto ret_free;

    /* increment the link count on success */
    ino->i_links_count++;
    write_inode(fs, ino, target->inode);

    ret = 0;
ret_free:
    kfree(ino);
    return ret;
}

static int ext2fs_symlink(fs_node_t *node, const char *name, const char *target) {
    ext2_fs_t *fs = node->device;
    int ret = 0;

    /* allocate an inode */
    uint32_t newino_no = alloc_inode(fs);
    if(!newino_no) return -ENOSPC;
    ext2_inode_t *newino = kmalloc(fs->inode_sz);
    memset(newino, 0, fs->inode_sz);

    /* TODO: actual time */
    newino->i_atime = newino->i_ctime = newino->i_mtime = 0;
    newino->i_dtime = 0;

    /* TODO: users? */
    newino->i_uid = 0;
    newino->i_gid = 0;

    newino->i_links_count = 1;
    newino->i_mode = EXT2_TYPE_LINK | 0777;
    /* all other fields are zero */

    /* create directory entry */
    if((ret = create_dirent(node, newino_no, name, EXT2_DIRENT_TYPE_LINK))) {
        /* free the new inode on failure */
        free_inode(fs, newino_no);
        goto ret_free;
    }

    /* write the actual link */
    size_t target_len = strlen(target);
    if(target_len <= sizeof newino->i_block) {
        memcpy(newino->i_block, target, target_len);
        newino->i_size = target_len;
        write_inode(fs, newino, newino_no);
    } else {
        /* this will write the inode, since the size will change */
        write_buffer_inode(fs, newino, newino_no, 0, target_len, (uint8_t *)target);
    }

    ret = 0;
ret_free:
    kfree(newino);
    return ret;
}

static int ext2fs_chmod(fs_node_t *node, mode_t mode) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    ino->i_mode &= ~07777;
    ino->i_mode |= (uint16_t)mode;

    write_inode(fs, ino, node->inode);

    return 0;
}

static int ext2fs_chown(fs_node_t *node, uid_t uid, gid_t gid) {
    ext2_fs_t *fs = node->device;
    ext2_inode_t *ino = read_inode(fs, node->inode);

    ino->i_uid = (uint16_t)uid;
    ino->i_gid = (uint16_t)gid;

    write_inode(fs, ino, node->inode);

    return 0;
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
    if((ino->i_mode & EXT2_TYPE_MASK) != EXT2_TYPE_DIR) {
        DPRINTF(CRITICAL, "root inode is not a directory!");
        return 1;
    }
    fnode->flags = FS_FLAG_IFDIR | FS_FLAG_MOUNT;

    fnode->open = ext2fs_open;
    fnode->close = ext2fs_close;
    fnode->readdir = ext2fs_readdir;
    fnode->finddir = ext2fs_finddir;
    fnode->mknod = ext2fs_mknod;
    fnode->unlink = ext2fs_unlink;
    fnode->link = ext2fs_link;
    fnode->symlink = ext2fs_symlink;
    fnode->chmod = ext2fs_chmod;
    fnode->chown = ext2fs_chown;
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
    /* temporary block size to read superblock correctly */
    fs->block_sz = 1024;
    ext2_superblock_t *sb = kmalloc(fs->block_sz);
    fs->sb = sb;
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
