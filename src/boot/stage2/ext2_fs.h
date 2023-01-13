#ifndef EXT2_FS_H_
#define EXT2_FS_H_

#include "stdint.h"

/* superblock */
#define EXT2_STATE_CLEAN  1
#define EXT2_STATE_ERRORS 2

#define EXT2_ERROR_IGNORE     1
#define EXT2_ERROR_RO_REMOUNT 2
#define EXT2_ERROR_PANIC      3

#define EXT2_OS_LINUX   0
#define EXT2_OS_HURD    1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_BSDLITE 4

#define EXT2_FEATURE_COMPAT_DIR_PREALLOC 0x0001
#define EXT2_FEATURE_COMPAT_AFS          0x0002
#define EXT2_FEATURE_COMPAT_JOURNAL      0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR     0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE 0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX    0x0020

#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE    0x0002
#define EXT2_FEATURE_INCOMPAT_RECOVER     0x0004
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008

#define EXT2_FEATURE_ROCOMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_ROCOMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_ROCOMPAT_BTREE_DIR    0x0004

/* inode */
#define EXT2_TYPE_FIFO  0x1000
#define EXT2_TYPE_CHAR  0x2000
#define EXT2_TYPE_DIR   0x4000
#define EXT2_TYPE_BLOCK 0x6000
#define EXT2_TYPE_REG   0x8000
#define EXT2_TYPE_LINK  0xa000
#define EXT2_TYPE_SOCK  0xc000

#define EXT2_SUID 04000
#define EXT2_SGID 02000
#define EXT2_SVTX 01000
#define EXT2_RUSR 00400
#define EXT2_WUSR 00200
#define EXT2_XUSR 00100
#define EXT2_RGRP 00040
#define EXT2_WGRP 00020
#define EXT2_XGRP 00010
#define EXT2_ROTH 00004
#define EXT2_WOTH 00002
#define EXT2_XOTH 00001

#define EXT2_FL_SECRM        0x00000001 /* secure deletion */
#define EXT2_FL_UNRM         0x00000002 /* undelete */
#define EXT2_FL_COMPR        0x00000004 /* compress file */
#define EXT2_FL_SYNC         0x00000008 /* synchronous updates */
#define EXT2_FL_IMMUTABLE    0x00000010 /* immutable file */
#define EXT2_FL_APPEND       0x00000020 /* only append */
#define EXT2_FL_NODUMP       0x00000040 /* do not dump file */
#define EXT2_FL_NOATIME      0x00000080 /* do not update atime */

#define EXT2_FL_BTREE        0x00001000 /* btree format dir */
#define EXT2_FL_AFS          0x00002000 /* no idea */
#define EXT2_FL_JOURNAL_DATA 0x00002000 /* file should be journaled */

/* various constants */
#define EXT2_NDIR_BLOCKS 12
#define EXT2_SIND_BLOCK  (EXT2_NDIR_BLOCKS+1)
#define EXT2_DIND_BLOCK  (EXT2_SIND_BLOCK+1)
#define EXT2_TIND_BLOCK  (EXT2_DIND_BLOCK+1)
#define EXT2_N_BLOCKS    (EXT2_TIND_BLOCK+1)

#define EXT2_MAGIC       0xef53

#define EXT2_BAD_INO     1
#define EXT2_ROOT_INO    2

#define EXT2_MIN_BLOCK_LOG_SIZE  10
#define EXT2_MAX_BLOCK_LOG_SIZE  16
#define EXT2_MIN_BLOCK_SIZE      (1<<EXT2_MIN_BLOCK_LOG_SIZE)
#define EXT2_MAX_BLOCK_SIZE      (1<<EXT2_MAX_BLOCK_LOG_SIZE)

#define EXT2_BLOCK_SIZE(s)       (EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#define EXT2_BLOCK_SIZE_BITS(s)  ((s)->s_log_block_size + 10)
#define EXT2_INODE_SIZE(s)       (((s)->s_rev_level == 0) \
    ? 128 : (s)->s_inode_size)
#define EXT2_FIRST_INO(s)        (((s)->s_rev_level == 0) \
    ? 11 : (s)->s_first_ino)
#define EXT2_ADDR_PER_BLOCK(s)   (EXT2_BLOCK_SIZE(s) / sizeof(uint32_t))
#define EXT2_BLOCK_TO_SECTOR(s)  (1<<(EXT2_BLOCK_SIZE_BITS(s)-9))

struct ext2_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_res_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_cluster_size;
    uint32_t s_blocks_per_group;
    uint32_t s_clusters_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    int16_t  s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;                   /* see EXT2_STATE_* */
    uint16_t s_errors;                  /* see EXT2_ERROR_* */
    uint16_t s_minor_rev_level;
    uint32_t s_last_check;
    uint32_t s_check_interval;
    uint32_t s_creator_os;              /* see EXT2_OS_* */
    uint32_t s_rev_level;
    uint16_t s_def_res_uid;
    uint16_t s_def_res_gid;

    /* extended superblock */
    uint32_t s_first_inode;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;          /* see EXT2_FEATURE_COMPAT_* */
    uint32_t s_feature_incompat;        /* see EXT2_FEATURE_INCOMPAT_* */
    uint32_t s_feature_ro_compat;       /* see EXT2_FEATURE_ROCOMPAT_* */
    uint8_t  s_uuid[16];
    uint8_t  s_label[16];
    uint8_t  s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint8_t  s_reserved1[2];
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inode;
    uint32_t s_journal_device;
    uint32_t s_last_orphan;
};

struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint8_t  reserved[14];
};

struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;                /* in disk sectors, not ext2 blocks */
    uint32_t i_flags;
    uint8_t  osd1[4];
    uint32_t i_block[EXT2_N_BLOCKS];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_size_high;
    uint32_t i_faddr;
    uint8_t  osd2[12];
};

struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  type; /* high byte of name_len unless EXT2_INCOMPAT_FILETYPE */
    char     name[255];
};

#endif // EXT2_FS_H_
