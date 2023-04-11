#ifndef EXT2FS_EXT2_H_
#define EXT2FS_EXT2_H_

#include <ext2fs/ext2_fs.h>
#include <kernel/fs.h>
#include <stdint.h>

#define EXT2_FLAG_RW         0x0001
#define EXT2_FLAG_VERBOSE    0x0002

typedef struct ext2_superblock ext2_superblock_t;
typedef struct ext2_group_desc ext2_group_desc_t;
typedef struct ext2_inode      ext2_inode_t;
typedef struct ext2_dir_entry  ext2_dir_entry_t;

typedef struct ext2_fs {
    ext2_superblock_t *sb;        /* superblock */
    ext2_group_desc_t *bgdt;      /* block group descriptor table */

    fs_node_t *root_fnode;        /* root fs node */
    fs_node_t *dev;               /* block device */

    uint32_t block_sz;            /* size of one block in bytes */
    uint32_t inodes_per_group;    /* inodes per block group */
    uint32_t bgdt_sz;             /* block group count */
    uint16_t inode_sz;            /* size of one inode in bytes */
    uint16_t fsblocks_per_block;  /* number of block device blocks per EXT2 block */
    uint8_t  bgdt_block_sz;       /* number of blocks in bgdt */
    uint8_t  bgdt_off;            /* starting block of the bgdt */

    unsigned flags;               /* mount flags */
} ext2_fs_t;

void ext2fs_init(void);

#endif // EXT2FS_EXT2_H_
