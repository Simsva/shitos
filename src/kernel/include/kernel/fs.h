#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/* fs_node::flags */
/* first three bits are reserved for the filetype */
#define FS_FLAG_FILE  0x01
#define FS_FLAG_DIR   0x02
#define FS_FLAG_CHAR  0x03
#define FS_FLAG_BLOCK 0x04
#define FS_FLAG_PIPE  0x05
#define FS_FLAG_LINK  0x06
#define FS_FLAG_MOUNT 0x08    /* is mountpoint? */

/* TODO: move to better locations */
typedef uint32_t off_t;
typedef uint32_t ino_t;

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;

    char d_name[256];
};

struct fs_node {
    char name[256];
    uint32_t mask;
    uint32_t uid, gid;
    uint32_t flags;
    ino_t inode;
    size_t sz;
    uintptr_t reserved;
    struct fs_node *ptr;

    ssize_t (*read)(struct fs_node *, off_t, size_t, uint8_t *);
    ssize_t (*write)(struct fs_node *, off_t, size_t, uint8_t *);
    void (*open)(struct fs_node *);
    void (*close)(struct fs_node *);
    struct dirent *(*readdir)(struct fs_node *, off_t);
    struct fs_node *(*finddir)(struct fs_node *, char *);
};

extern struct fs_node *fs_root;

ssize_t fs_read(struct fs_node *node, off_t off, size_t sz, uint8_t *buf);
ssize_t fs_write(struct fs_node *node, off_t off, size_t sz, uint8_t *buf);
void fs_open(struct fs_node *node, int flags);
void fs_close(struct fs_node *node);
struct dirent *fs_readdir(struct fs_node *node, off_t idx);
struct fs_node *fs_finddir(struct fs_node *node, char *name);

#warning "remove this"
void fs_init_test(void);

#endif // KERNEL_FS_H_
