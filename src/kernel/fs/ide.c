/**
 * @brief (P)ATA / IDE disk driver
 */
#include <kernel/fs.h>
#include <kernel/args.h>
#include <kernel/pci.h>
#include <kernel/kmem.h>
#include <kernel/vmem.h>

#include <kernel/arch/i386/ports.h>
#include <kernel/arch/i386/irq.h>

#include <stdio.h>
#include <string.h>

#define ATA_STATUS_BSY            0x80    /* busy */
#define ATA_STATUS_DRDY           0x40    /* drive ready */
#define ATA_STATUS_DF             0x20    /* drive write fault */
#define ATA_STATUS_DSC            0x10    /* drive seek complete */
#define ATA_STATUS_DRQ            0x08    /* data request ready */
#define ATA_STATUS_CORR           0x04    /* corrected data */
#define ATA_STATUS_IDX            0x02    /* index */
#define ATA_STATUS_ERR            0x01    /* error */

#define ATA_ERR_BBK               0x80    /* bad block */
#define ATA_ERR_UNC               0x40    /* uncorrectable data */
#define ATA_ERR_MC                0x20    /* media changed */
#define ATA_ERR_IDNF              0x10    /* ID mark not found */
#define ATA_ERR_MCR               0x08    /* media change request */
#define ATA_ERR_ABRT              0x04    /* command aborted */
#define ATA_ERR_TK0NF             0x02    /* track 0 not found */
#define ATA_ERR_AMNF              0x01    /* no address mark */

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xc8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xca
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xe7
#define ATA_CMD_CACHE_FLUSH_EXT   0xea
#define ATA_CMD_PACKET            0xa0
#define ATA_CMD_IDENTIFY_PACKET   0xa1
#define ATA_CMD_IDENTIFY          0xec

#define ATAPI_CMD_READ            0xa8
#define ATAPI_CMD_EJECT           0x1b

#define ATA_IDENT_DEVICETYPE      0
#define ATA_IDENT_CYLINDERS       2
#define ATA_IDENT_HEADS           6
#define ATA_IDENT_SECTORS         12
#define ATA_IDENT_SERIAL          20
#define ATA_IDENT_MODEL           54
#define ATA_IDENT_CAPABILITIES    98
#define ATA_IDENT_FIELDVALID      106
#define ATA_IDENT_MAX_LBA         120
#define ATA_IDENT_COMMANDSETS     164
#define ATA_IDENT_MAX_LBA_EXT     200

#define IDE_ATA                   0x00
#define IDE_ATAPI                 0x01

#define ATA_MASTER                0x00
#define ATA_SLAVE                 0x01

#define ATA_REG_DATA              0x00
#define ATA_REG_ERROR             0x01
#define ATA_REG_FEATURES          0x01
#define ATA_REG_SECCOUNT0         0x02
#define ATA_REG_LBA0              0x03
#define ATA_REG_LBA1              0x04
#define ATA_REG_LBA2              0x05
#define ATA_REG_HDDEVSEL          0x06
#define ATA_REG_COMMAND           0x07
#define ATA_REG_STATUS            0x07
#define ATA_REG_SECCOUNT1         0x08
#define ATA_REG_LBA3              0x09
#define ATA_REG_LBA4              0x0a
#define ATA_REG_LBA5              0x0b
#define ATA_REG_CONTROL           0x0c
#define ATA_REG_ALTSTATUS         0x0c
#define ATA_REG_DEVADDRESS        0x0d

/* channels */
#define ATA_PRIMARY               0x00
#define ATA_SECONDARY             0x01

/* directions */
#define ATA_READ                  0x00
#define ATA_WRITE                 0x01

#define ATA_SECTOR_SZ             512
#define ATA_BLOCK_SZ              PAGE_SIZE
#define ATA_SECTORS_PER_BLOCK     (ATA_BLOCK_SZ/ATA_SECTOR_SZ)

typedef union {
    uint64_t raw;
    struct {
        uint32_t lo, hi;
    };
} lba48_t;

typedef struct ata_identify {
    uint16_t flags;
    uint16_t unused1[9];
    char serial[20];
    uint16_t unused2[3];
    char firmware[8];
    char model[40];
    uint16_t sectors_per_int;
    uint16_t unused3;
    uint16_t capabilities[2];
    uint16_t unused4[2];
    uint16_t valid_ext_data;
    uint16_t unused5[5];
    uint16_t size_of_rw_mult;
    uint32_t sectors_28;
    uint16_t unused6[38];
    lba48_t sectors_48;
    uint16_t unused7[152];
} __attribute__((packed)) ata_identify_t;

typedef struct {
    uintptr_t offset;
    uint16_t bytes;
    uint16_t last;
} prd_t;

typedef struct ata_device {
    uint16_t io_base;
    uint16_t control;
    uint8_t slave;
    uint8_t is_atapi;
    ata_identify_t identity;
    prd_t *dma_prdt;
    uintptr_t dma_prdt_phys;
    uint8_t *dma_start;
    uintptr_t dma_start_phys;
    uint32_t bar4;
    uint32_t atapi_lba;
    uint32_t atapi_sector_size;
} ata_device_t;

static pci_device_t ide_pci = { 0 };
static char ata_drive_char = 'a';

static ata_device_t ata_primary_master   = { .io_base = 0x1F0, .control = 0x3F6, .slave = 0 };
static ata_device_t ata_primary_slave    = { .io_base = 0x1F0, .control = 0x3F6, .slave = 1 };
static ata_device_t ata_secondary_master = { .io_base = 0x170, .control = 0x376, .slave = 0 };
static ata_device_t ata_secondary_slave  = { .io_base = 0x170, .control = 0x376, .slave = 1 };

static void ata_io_wait(ata_device_t *dev);
static uint8_t ata_status_wait(ata_device_t *dev, int timeout);
static void ata_soft_reset(ata_device_t *dev);
static void ata_device_detect(ata_device_t *dev);
static fs_node_t *ata_device_create(ata_device_t *dev, char drive_char);
static void find_ide_pci(pci_device_t dev, uint16_t vnid, uint16_t dvid, void *extra);

static void ata_device_init(ata_device_t *dev);
static void ata_device_destroy(ata_device_t *dev);
static void ata_device_read_block(ata_device_t *dev, lba48_t lba, uint8_t *buf);
static void ata_device_read_block_internal(ata_device_t *dev, lba48_t lba);
static void ata_device_write_block(ata_device_t *dev, lba48_t lba, uint8_t *buf);
static void ata_device_write_block_internal(ata_device_t *dev, lba48_t lba);
static void ata_irq_handler(struct int_regs *r);

static off_t ata_max_offset(ata_device_t *dev);
static ssize_t ata_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
static ssize_t ata_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);

void ide_init(void);

#define DPRINTF(fmt, ...) if(kernel_args.boot_options & BOOT_OPT_VERBOSE) { \
        printf(fmt, __VA_ARGS__); }
#define DPUTS(str) if(kernel_args.boot_options & BOOT_OPT_VERBOSE) { \
        puts(str); }

static void ata_io_wait(ata_device_t *dev) {
    inportb(dev->io_base + ATA_REG_ALTSTATUS);
    inportb(dev->io_base + ATA_REG_ALTSTATUS);
    inportb(dev->io_base + ATA_REG_ALTSTATUS);
    inportb(dev->io_base + ATA_REG_ALTSTATUS);
}

static uint8_t ata_status_wait(ata_device_t *dev, int timeout) {
    uint8_t status;
    if(timeout > 0)
        for(int i = 0;
            (status = inportb(dev->io_base + ATA_REG_STATUS)) & ATA_STATUS_BSY
                && (i < timeout);
            i++);
    else
        while((status = inportb(dev->io_base + ATA_REG_STATUS)) & ATA_STATUS_BSY);
    return status;
}

static void ata_soft_reset(ata_device_t *dev) {
    outportb(dev->control, 0x04);
    ata_io_wait(dev);
    outportb(dev->control, 0x00);
}

static void ata_device_detect(ata_device_t *dev) {
    ata_soft_reset(dev);
    ata_io_wait(dev);
    /* select drive */
    outportb(dev->io_base + ATA_REG_HDDEVSEL, 0xa0 | dev->slave << 4);
    ata_io_wait(dev);
    if(ata_status_wait(dev, 10000) & ATA_STATUS_ERR) {
        DPRINTF("ide: %04X[%u] not ATA\n", dev->io_base, dev->slave);
        return;
    }

    uint8_t cl = inportb(dev->io_base + ATA_REG_LBA1);
    uint8_t ch = inportb(dev->io_base + ATA_REG_LBA2);

    if(cl == 0xff && ch == 0xff) {
        DPRINTF("ide: %04X[%u] nothing found\n", dev->io_base, dev->slave);
        return;
    }
    if((cl == 0x00 && ch == 0x00)
       || (cl == 0x3c && ch == 0xc3)) {
        /* PATA or emulated SATA */
        DPRINTF("ide: %04X[%u] PATA or emu SATA\n", dev->io_base, dev->slave);

        ata_device_init(dev);

        off_t sectors = ata_max_offset(dev);
        if(sectors == 0) {
            DPUTS("ide: max offset == 0");
            ata_device_destroy(dev);
            return;
        }

        char devname[64];
        snprintf(devname, sizeof devname, "/dev/ad%c", ata_drive_char);
        fs_node_t *fnode = ata_device_create(dev, ata_drive_char++);
        vfs_mount(devname, fnode);
        fnode->sz = sectors;

        return;
    }
    if((cl == 0x14 && ch == 0xeb)
       || (cl == 0x69 && ch == 0x96)) {
        /* ATAPI */
        DPRINTF("ide: %04X[%u] ATAPI\n", dev->io_base, dev->slave);
        return;
    }

    DPRINTF("ide: %04X[%u] not recognized\n", dev->io_base, dev->slave);
    return;
}

static fs_node_t *ata_device_create(ata_device_t *dev, char drive_char) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    snprintf(fnode->name, sizeof fnode->name, "atadev%c", drive_char);
    fnode->device = dev;
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0660;
    fnode->flags = FS_FLAG_IFBLK;

    fnode->read = ata_read;
    fnode->write = ata_write;

    return fnode;
}

static void find_ide_pci(pci_device_t dev, __unused uint16_t vnid, __unused uint16_t dvid, void *extra) {
    *(pci_device_t *)extra = dev;
}

static void ata_device_init(ata_device_t *dev) {
    outportb(dev->io_base + 1, 1);
    outportb(dev->control, 0);

    /* select drive */
    outportb(dev->io_base + ATA_REG_HDDEVSEL, 0xa0 | dev->slave << 4);
    ata_io_wait(dev);

    /* ATA identify */
    outportb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait(dev);

    inportb(dev->io_base + ATA_REG_STATUS);
    ata_io_wait(dev);
    ata_status_wait(dev, -1);

    /* save identity */
    uint16_t *buf = (uint16_t *)&dev->identity;
    for(uint16_t i = 0; i < 256; i++)
        buf[i] = inportw(dev->io_base);

    /* fix endianness in model */
    uint8_t *p = (uint8_t *)&dev->identity.model;
    for(uint8_t i = 0; i < 39; i += 2) {
        uint8_t t = p[i+1];
        p[i+1] = p[i];
        p[i] = t;
    }

    dev->is_atapi = 0;
    /* TODO: alloc pages outside the heap maybe? */
    dev->dma_prdt = kmalloc_a(PAGE_SIZE); /* we only ever use one entry */
    dev->dma_prdt_phys = (uintptr_t)vmem_get_paddr(dev->dma_prdt);
    dev->dma_start = kmalloc_a(PAGE_SIZE);
    dev->dma_start_phys = (uintptr_t)vmem_get_paddr(dev->dma_start);

    dev->dma_prdt[0].offset = dev->dma_start_phys;
    dev->dma_prdt[0].bytes = ATA_BLOCK_SZ;
    dev->dma_prdt[0].last = 0x8000;

    uint32_t cmd_reg = pci_read_register(ide_pci, PCI_COMMAND, 4);
    if(!(cmd_reg & (1 << 2)))
        pci_write_register(ide_pci, PCI_COMMAND, 4, cmd_reg | (1 << 2));

    dev->bar4 = pci_read_register(ide_pci, PCI_BAR4, 4);
    if(dev->bar4 & 0x1)
        dev->bar4 &= 0xfffffffc;
}

/* only called on unused devices */
static void ata_device_destroy(ata_device_t *dev) {
    kfree(dev->dma_prdt);
    kfree(dev->dma_start);
}

/* TODO: caching */
static void ata_device_read_block(ata_device_t *dev, lba48_t lba, uint8_t *buf) {
    /* we only read one block at a time, so lba is really
     * the block address and not the ATA sector address */
    lba.raw *= ATA_SECTORS_PER_BLOCK;

    ata_device_read_block_internal(dev, lba);
    memcpy(buf, dev->dma_start, ATA_BLOCK_SZ);
}

static void ata_device_read_block_internal(ata_device_t *dev, lba48_t lba) {
    uint8_t status, dstatus;
    if(dev->is_atapi) return;

    ata_io_wait(dev);
    ata_status_wait(dev, -1);

    /* clear command register */
    outportb(dev->bar4, 0);
    /* set the PRDT (BAR4 + 4 to 7 bytes) */
    outportl(dev->bar4 + 4, dev->dma_prdt_phys);
    /* set read bit */
    outportb(dev->bar4, 1<<3);
    /* clear error and IRQ bit */
    status = inportb(dev->bar4 + 2);
    outportb(dev->bar4 + 2, status | (1<<1) | (1<<3));

    ata_status_wait(dev, -1);
    outportb(dev->io_base + ATA_REG_CONTROL, 0x00);
    /* select drive in LBA mode */
    outportb(dev->io_base + ATA_REG_HDDEVSEL, 0xe0 | dev->slave << 4);
    ata_io_wait(dev);
    ata_status_wait(dev, -1);
    outportb(dev->io_base + ATA_REG_FEATURES, 0x00);

    /* select high 24 bits of LBA (NOTE: uses 32-bit ints) */
    outportb(dev->io_base + ATA_REG_SECCOUNT0, 0);
    outportb(dev->io_base + ATA_REG_LBA0, (lba.lo & 0xff000000) >> 24);
    outportb(dev->io_base + ATA_REG_LBA1, (lba.hi & 0x000000ff));
    outportb(dev->io_base + ATA_REG_LBA2, (lba.hi & 0x0000ff00) >> 8);
    /* select low 24 bits of LBA */
    outportb(dev->io_base + ATA_REG_SECCOUNT0, ATA_SECTORS_PER_BLOCK);
    outportb(dev->io_base + ATA_REG_LBA0, (lba.lo & 0x000000ff) >> 0);
    outportb(dev->io_base + ATA_REG_LBA1, (lba.lo & 0x0000ff00) >> 8);
    outportb(dev->io_base + ATA_REG_LBA2, (lba.lo & 0x00ff0000) >> 16);

    /* wait for drive to be ready */
    do status = inportb(dev->io_base + ATA_REG_STATUS);
    while((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRDY));

    /* DMA read extended (48-bit LBA) */
    outportb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_READ_DMA_EXT);
    ata_io_wait(dev);
    /* set read and start/stop bit */
    outportb(dev->bar4, (1<<3) | (1<<0));

    /* wait for read to finish */
    for(;;) {
        status = inportb(dev->bar4 + 2);
        dstatus = inportb(dev->io_base + ATA_REG_STATUS);
        /* IRQ bit not set */
        if(!(status & (1<<2)))
            continue;
        /* drive isn't busy */
        if(!(dstatus & ATA_STATUS_BSY))
            break;
    }

    /* reset IRQ and error bit or something idk */
    status = inportb(dev->bar4 + 2);
    outportb(dev->bar4 + 2, status | (1<<2) | (1<<1));
}

static void ata_device_write_block(ata_device_t *dev, lba48_t lba, uint8_t *buf) {
    /* we only read one block at a time, so lba is really
     * the block address and not the ATA sector address */
    lba.raw *= ATA_SECTORS_PER_BLOCK;

    memcpy(dev->dma_start, buf, ATA_BLOCK_SZ);
    ata_device_write_block_internal(dev, lba);
}

static void ata_device_write_block_internal(ata_device_t *dev, lba48_t lba) {
    uint8_t status;
    if(dev->is_atapi) return;

    ata_io_wait(dev);
    ata_status_wait(dev, -1);

    /* clear command register (including read/write bit) */
    outportb(dev->bar4, 0);
    /* set the PRDT (BAR4 + 4 to 7 bytes) */
    outportl(dev->bar4 + 4, dev->dma_prdt_phys);
    /* clear error and IRQ bit */
    status = inportb(dev->bar4 + 2);
    outportb(dev->bar4 + 2, status | (1<<1) | (1<<3));

    ata_status_wait(dev, -1);
    outportb(dev->io_base + ATA_REG_CONTROL, 0x00);
    /* select drive in LBA mode */
    outportb(dev->io_base + ATA_REG_HDDEVSEL, 0xe0 | dev->slave << 4);
    ata_io_wait(dev);
    ata_status_wait(dev, -1);
    outportb(dev->io_base + ATA_REG_FEATURES, 0x00);

    /* select high 24 bits of LBA (NOTE: uses 32-bit ints) */
    outportb(dev->io_base + ATA_REG_SECCOUNT0, 0);
    outportb(dev->io_base + ATA_REG_LBA0, (lba.lo & 0xff000000) >> 24);
    outportb(dev->io_base + ATA_REG_LBA1, (lba.hi & 0x000000ff));
    outportb(dev->io_base + ATA_REG_LBA2, (lba.hi & 0x0000ff00) >> 8);
    /* select low 24 bits of LBA */
    outportb(dev->io_base + ATA_REG_SECCOUNT0, ATA_SECTORS_PER_BLOCK);
    outportb(dev->io_base + ATA_REG_LBA0, (lba.lo & 0x000000ff) >> 0);
    outportb(dev->io_base + ATA_REG_LBA1, (lba.lo & 0x0000ff00) >> 8);
    outportb(dev->io_base + ATA_REG_LBA2, (lba.lo & 0x00ff0000) >> 16);

    /* wait for drive to be ready */
    do status = inportb(dev->io_base + ATA_REG_STATUS);
    while((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRDY));

    /* DMA write extended (48-bit LBA) */
    outportb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_DMA_EXT);
    ata_io_wait(dev);
    /* set start/stop bit */
    outportb(dev->bar4, (1<<0));

    ata_io_wait(dev);
    ata_status_wait(dev, -1);

    /* reset IRQ and error bit or something idk */
    status = inportb(dev->bar4 + 2);
    outportb(dev->bar4 + 2, status | (1<<2) | (1<<1));
}

static void ata_irq_handler(struct int_regs *r) {
    ata_device_t *dev = r->int_no == 14
        ? &ata_primary_master : &ata_secondary_master;
    /* reset start/stop bit */
    outportb(dev->bar4, 1<<1);

    /* TODO: error handling */
    inportb(dev->io_base + ATA_REG_STATUS);
}

static off_t ata_max_offset(ata_device_t *dev) {
    lba48_t sectors = dev->identity.sectors_48;
    if(!sectors.raw) {
        sectors.raw = dev->identity.sectors_28;
    }
    return sectors.raw * ATA_SECTOR_SZ;
}

static ssize_t ata_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    ata_device_t *dev = node->device;
    lba48_t start_block = (lba48_t){ .raw = off / ATA_BLOCK_SZ };
    lba48_t end_block = (lba48_t){ .raw = (off + sz - 1) / ATA_BLOCK_SZ };
    size_t buf_offset = 0;

    off_t max_off = ata_max_offset(dev);
    if(off > max_off) return 0;
    if(off + (ssize_t)sz > max_off)
        sz = max_off - off;

    /* if start is not on a block boundary
     * or if total size is less than one block */
    if(off % ATA_BLOCK_SZ || sz < ATA_BLOCK_SZ) {
        size_t prefix_sz = ATA_BLOCK_SZ - (off % ATA_BLOCK_SZ);
        if(prefix_sz > sz) prefix_sz = sz;

        uint8_t *tmp = kmalloc(ATA_BLOCK_SZ);
        ata_device_read_block(dev, start_block, tmp);

        memcpy(buf, tmp + (off % ATA_BLOCK_SZ), prefix_sz);

        kfree(tmp);
        buf_offset += prefix_sz;
        /* first block is read */
        start_block.raw++;
    }

    /* if end is not on a block boundary (and there are actually blocks left to
     * read after the first case)
     * if the total size fits within the start block, then do not run this even
     * though the end is not on a boundary */
    if((off + sz) % ATA_BLOCK_SZ && start_block.raw <= end_block.raw) {
        size_t postfix_sz = (off + sz) % ATA_BLOCK_SZ;
        uint8_t *tmp = kmalloc(ATA_BLOCK_SZ);
        ata_device_read_block(dev, end_block, tmp);

        memcpy(buf + sz - postfix_sz, tmp, postfix_sz);

        kfree(tmp);
        /* last block is read */
        end_block.raw--;
    }

    /* read the remaining blocks */
    while(start_block.raw <= end_block.raw) {
        ata_device_read_block(dev, start_block, buf + buf_offset);
        buf_offset += ATA_BLOCK_SZ;
        start_block.raw++;
    }

    return sz;
}

static ssize_t ata_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    ata_device_t *dev = node->device;
    lba48_t start_block = (lba48_t){ .raw = off / ATA_BLOCK_SZ };
    lba48_t end_block = (lba48_t){ .raw = (off + sz - 1) / ATA_BLOCK_SZ };
    size_t buf_offset = 0;

    off_t max_off = ata_max_offset(dev);
    if(off > max_off) return 0;
    if(off + (ssize_t)sz > max_off)
        sz = max_off - off;

    /* if start is not on a block boundary
     * or if total size is less than one block */
    if(off % ATA_BLOCK_SZ || sz < ATA_BLOCK_SZ) {
        size_t prefix_sz = ATA_BLOCK_SZ - (off % ATA_BLOCK_SZ);
        if(prefix_sz > sz) prefix_sz = sz;

        uint8_t *tmp = kmalloc(ATA_BLOCK_SZ);
        ata_device_read_block(dev, start_block, tmp);

        memcpy(tmp + (off % ATA_BLOCK_SZ), buf, prefix_sz);
        ata_device_write_block(dev, start_block, tmp);

        kfree(tmp);
        buf_offset += prefix_sz;
        /* first block is written */
        start_block.raw++;
    }

    /* if end is not on a block boundary */
    if((off + sz) % ATA_BLOCK_SZ && start_block.raw <= end_block.raw) {
        size_t postfix_sz = (off + sz) % ATA_BLOCK_SZ;
        uint8_t *tmp = kmalloc(ATA_BLOCK_SZ);
        ata_device_read_block(dev, end_block, tmp);

        memcpy(tmp, buf + sz - postfix_sz, postfix_sz);
        ata_device_write_block(dev, end_block, tmp);

        kfree(tmp);
        /* last block is written */
        end_block.raw--;
    }

    /* write the remaining blocks */
    while(start_block.raw <= end_block.raw) {
        ata_device_write_block(dev, start_block, buf + buf_offset);
        buf_offset += ATA_BLOCK_SZ;
        start_block.raw++;
    }

    return sz;
}

void ide_init(void) {
    /* find an IDE controller */
    pci_scan(find_ide_pci, 0x0101, &ide_pci);
    if(!ide_pci.raw) {
        puts("ide: no IDE controller found");
        return;
    }

    irq_handler_install(14, ata_irq_handler);
    irq_handler_install(15, ata_irq_handler);

    ata_device_detect(&ata_primary_master);
    ata_device_detect(&ata_primary_slave);
    ata_device_detect(&ata_secondary_master);
    ata_device_detect(&ata_secondary_slave);
}
