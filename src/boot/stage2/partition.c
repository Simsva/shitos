#include "partition.h"

#include "ext2_fs.h"
#include "string.h"
#include "v86.h"
#include "tm_io.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void ext2_read_inode(struct partition_entry *entry, uint8_t drive_num,
                     struct ext2_superblock *sb, struct ext2_group_desc *bgdt,
                     uint32_t mem_buf, uint32_t inode, struct ext2_inode *buf);

void ext2_read_file(struct partition_entry *entry, uint8_t drive_num,
                    struct ext2_superblock *sb, struct ext2_inode *inode,
                    uint32_t mem_buf);

void xread(uint64_t lba, uint32_t segment, uint32_t offset,
           uint8_t drive_num, uint8_t num_sectors) {
    /* xread from stage1 */
    v86.ctl = V86F_ADDR;
    v86.addr = 0x075a;
    /* read lba */
    v86.ecx = lba>>32;
    v86.eax = lba;
    /* into segment:offset */
    v86.es  = segment;
    v86.ebx = offset;
    /* num blocks and drive num */
    v86.edx = num_sectors<<8 | drive_num;
    v86int();
}

int8_t partition_ext2_parse(struct partition_entry *entry, uint8_t drive_num) {
    struct ext2_superblock *sb = (struct ext2_superblock *)MEM_ESB;
    struct ext2_group_desc *bgdt;
    struct ext2_inode root_inode;
    uint32_t bg_lba, bgdt_size, i, mem_buf_head;
    uint16_t sectors_per_block, inode_size;
    void *dir_entry;
    char name_buf[256];

    /* read ext2 superblock */
    xread(entry->start_lba+2, 0, MEM_ESB, drive_num, 2);
    if(sb->s_magic != EXT2_MAGIC) return PARSE_EXT2_NOTEXT2;

    /* fs information */
    sectors_per_block = EXT2_BLOCK_TO_SECTOR(sb);
    inode_size = EXT2_INODE_SIZE(sb);
    bgdt_size = MAX((sb->s_blocks_count + sb->s_blocks_per_group-1)
                    / sb->s_blocks_per_group,
                    (sb->s_inodes_count + sb->s_inodes_per_group-1)
                    / sb->s_inodes_per_group);

    tm_printf("ipg:%u\tinodesz:%u\tspb:%u\tbgdt_sz:%u\n",
              sb->s_inodes_per_group, inode_size, sectors_per_block, bgdt_size);

    /* read block group descriptor table */
    bg_lba = entry->start_lba + sectors_per_block;
    if(sectors_per_block == 2) bg_lba += sectors_per_block;
    xread(bg_lba, 0, MEM_BUF, drive_num,
          (bgdt_size*sizeof(struct ext2_group_desc) + 511)/512);
    mem_buf_head = MEM_BUF
        + 512*((bgdt_size*sizeof(struct ext2_group_desc) + 511)/512);
    bgdt = (struct ext2_group_desc *)(MEM_BUF);

    /* read root dir inode */
    ext2_read_inode(entry, drive_num, sb, bgdt,
                    mem_buf_head, EXT2_ROOT_INO, &root_inode);
    tm_printf("root mode: 0%o\tblocks: %u\n",
              root_inode.i_mode&0777, root_inode.i_blocks/sectors_per_block);

    ext2_read_file(entry, drive_num, sb, &root_inode, mem_buf_head);

#define D(p) ((struct ext2_dir_entry *)p)
    dir_entry = (void *)mem_buf_head;
    for(i = 0; D(dir_entry)->rec_len; ++i) {
        memcpy(name_buf, D(dir_entry)->name, D(dir_entry)->name_len);
        name_buf[D(dir_entry)->name_len] = '\0';
        tm_printf("root entry %u name: %s\n", i, name_buf);

        if(strcmp(name_buf, "shitos.elf") == 0) goto file_found;

        dir_entry += D(dir_entry)->rec_len;
    }
    return PARSE_EXT2_NOFILE;
file_found:

    /* read contents of shitos.elf */
    ext2_read_inode(entry, drive_num, sb, bgdt, 0x5000,
                    D(dir_entry)->inode, &root_inode);
    ext2_read_file(entry, drive_num, sb, &root_inode, mem_buf_head);

    tm_printf("shitos.elf: %s\n", mem_buf_head);
#undef D

    return PARSE_EXT2_SUCCESS;
}

/* private functions */
void ext2_read_inode(struct partition_entry *entry, uint8_t drive_num,
                     struct ext2_superblock *sb, struct ext2_group_desc *bgdt,
                     uint32_t mem_buf, uint32_t inode, struct ext2_inode *buf) {
    uint16_t block_group;
    uint32_t bg_index, lba;

    block_group      = (inode - 1) / sb->s_inodes_per_group;
    bg_index         = (inode - 1) % sb->s_inodes_per_group;

    lba = entry->start_lba
        + EXT2_BLOCK_TO_SECTOR(sb)*bgdt[block_group].bg_inode_table
        + (bg_index*EXT2_INODE_SIZE(sb)) / 512;

    /* reads 2 * inode size to make sure the full inode gets read */
    xread(lba, 0, mem_buf, drive_num, (2*EXT2_INODE_SIZE(sb) + 511)/512);
    memcpy(buf, (uint8_t *)(mem_buf + (bg_index*EXT2_INODE_SIZE(sb)) % 512),
           sizeof(struct ext2_inode));
}

void ext2_read_file(struct partition_entry *entry, uint8_t drive_num,
                    struct ext2_superblock *sb, struct ext2_inode *inode,
                    uint32_t mem_buf) {
    uint8_t i;

    /* FIXME: does not handle indirect blocks,
     * even though that will never happen */
    for(i = 0; i < inode->i_blocks/EXT2_BLOCK_TO_SECTOR(sb); ++i) {
        tm_printf("reading block %u at %x\n", i, inode->i_block[i]);
        xread(entry->start_lba + inode->i_block[i]*EXT2_BLOCK_TO_SECTOR(sb),
              0, mem_buf + 512*i*EXT2_BLOCK_TO_SECTOR(sb),
              drive_num, EXT2_BLOCK_TO_SECTOR(sb));
    }
}
