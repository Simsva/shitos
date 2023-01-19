#ifndef PARTITION_H_
#define PARTITION_H_

#include "stdint.h"

#define MAX_BLK_SIZE 4096 /* maximum supported ext2 block size */

/* memory locations */
#define MEM_MBR 0x300
#define MEM_DRV 0x900     /* drive number placed here by stage1 */
#define MEM_ESB 0xa00     /* ext2 superblock */
#define MEM_BLK 0xe00     /* ext2 buffer for indirect blocks */
#define MEM_BUF (MEM_BLK + MAX_BLK_SIZE*3) /* disk buffer */

/* partition_ext2_parse error codes */
#define PARSE_EXT2_SUCCESS 0
#define PARSE_EXT2_NOTEXT2 1
#define PARSE_EXT2_NOFILE  2
#define PARSE_EXT2_TOOBIG  3

struct partition_entry {
    uint8_t boot;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t start_lba;
    uint32_t total_sectors;
} __attribute__((packed));

void xread(uint64_t lba, uint32_t segment, uint32_t offset,
           uint8_t drive_num, uint8_t num_sectors);
int8_t partition_ext2_parse(struct partition_entry *entry, uint8_t drive_num,
                            void **elf_addr);

#endif // PARTITION_H_
