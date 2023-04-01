#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

#include <kernel/tree.h>

/* fs_node::flags */
#define FS_TYPE_FILE      0x01
#define FS_TYPE_DIR       0x02
#define FS_TYPE_CHAR      0x04
#define FS_TYPE_BLOCK     0x08
#define FS_TYPE_PIPE      0x10
#define FS_TYPE_LINK      0x20

#define FS_FLAG_MOUNT     0x40    /* is mountpoint? */

#define PATH_SEPARATOR     '/'
#define PATH_SEPARATOR_STR "/"
#define PATH_CURRENT       "."
#define PATH_GO_UP         ".."

struct fs_node;

typedef ssize_t (*read_type_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef ssize_t (*write_type_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef void (*open_type_t)(struct fs_node *, unsigned);
typedef void (*close_type_t)(struct fs_node *);
typedef struct dirent *(*readdir_type_t)(struct fs_node *, off_t);
typedef struct fs_node *(*finddir_type_t)(struct fs_node *, char *);
typedef ssize_t (*readlink_type_t)(struct fs_node *, char *, size_t);

typedef struct fs_node {
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
    readlink_type_t readlink;

    struct fs_node *ptr;   /* alias pointer, for symlinks */
    intmax_t refcount;
    intmax_t links_count;  /* number of hard links to this node */
} fs_node_t;

struct vfs_entry {
    char *name;            /* name of entry */
    struct fs_node *file;  /* fs_node it points to */
    char *args;            /* mount arguments (including device file
                            * for physical filesystems) */
    char *fs_type;         /* filesystem type */
};

typedef fs_node_t *(*vfs_mount_t)(const char *, const char *);

extern tree_t *fs_tree;
extern fs_node_t *console_dev; /* TODO: move this */

ssize_t fs_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
ssize_t fs_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
void fs_open(fs_node_t *node, unsigned flags);
void fs_close(fs_node_t *node);
struct dirent *fs_readdir(fs_node_t *node, off_t idx);
fs_node_t *fs_finddir(fs_node_t *node, char *name);
int fs_readlink(fs_node_t *node, char *buf, size_t sz);

void vfs_install(void);
void random_install(void);
void zero_install(void);
void console_install(void);

int vfs_register_type(const char *type, vfs_mount_t mount);
void *vfs_mount(const char *path, fs_node_t *root);
int vfs_mount_type(const char *type, const char *arg, const char *mountpoint);
void vfs_map_directory(const char *path);

char *canonicalize_path(const char *cwd, const char *path, size_t *length);
fs_node_t *kopen(const char *path, unsigned flags);

#endif // KERNEL_FS_H_
