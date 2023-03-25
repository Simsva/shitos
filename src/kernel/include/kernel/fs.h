#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

/* fs_node::flags */
/* first three bits are reserved for the filetype */
#define FS_FLAG_TYPE_MASK 07
#define FS_TYPE_FILE      01
#define FS_TYPE_DIR       02
#define FS_TYPE_CHAR      03
#define FS_TYPE_BLOCK     04
#define FS_TYPE_PIPE      05
#define FS_TYPE_LINK      06

#define FS_FLAG_MOUNT     0x08    /* is mountpoint? */

struct fs_node;

typedef ssize_t (*read_type_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef ssize_t (*write_type_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef void (*open_type_t)(struct fs_node *, unsigned);
typedef void (*close_type_t)(struct fs_node *);
typedef struct dirent *(*readdir_type_t)(struct fs_node *, off_t);
typedef struct fs_node *(*finddir_type_t)(struct fs_node *, char *);

struct fs_node {
    char name[256];        /* filename */
    void *device;          /* device */
    mode_t mask;           /* permissions mask */
    uid_t uid;             /* owning user */
    gid_t gid;             /* owning group */
    uint32_t flags;        /* flags (type, etc.) */
    ino_t inode;           /* inode number */
    size_t sz;             /* size of file in bytes */
    uintptr_t reserved;    /* reserved for driver */
    unsigned open_flags;   /* flags passed to open */

    /* file operations */
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;

    struct fs_node *ptr;   /* alias pointer, for symlinks */
    intmax_t refcount;
};

extern struct fs_node *fs_root;

ssize_t fs_read(struct fs_node *node, off_t off, size_t sz, uint8_t *buf);
ssize_t fs_write(struct fs_node *node, off_t off, size_t sz, uint8_t *buf);
void fs_open(struct fs_node *node, unsigned flags);
void fs_close(struct fs_node *node);
struct dirent *fs_readdir(struct fs_node *node, off_t idx);
struct fs_node *fs_finddir(struct fs_node *node, char *name);

struct fs_node *kopen(const char *path, unsigned flags);

/* TODO: remove this */
void fs_init_test(void);

#endif // KERNEL_FS_H_
