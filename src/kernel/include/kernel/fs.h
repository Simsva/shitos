#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

#include <kernel/tree.h>

/* fs_node::flags */
/* first 4 bits reserved to filetype */
#define FS_FLAG_IFMT    017
#define FS_FLAG_IFDIR   004
#define FS_FLAG_IFCHR   002
#define FS_FLAG_IFBLK   006
#define FS_FLAG_IFREG   010
#define FS_FLAG_IFIFO   001
#define FS_FLAG_IFLNK   012
#define FS_FLAG_IFSOCK  014

#define FS_ISDIR(mode)  (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFDIR)
#define FS_ISCHR(mode)  (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFCHR)
#define FS_ISBLK(mode)  (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFBLK)
#define FS_ISREG(mode)  (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFREG)
#define FS_ISFIFO(mode) (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFIFO)
#define FS_ISLNK(mode)  (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFLNK)
#define FS_ISSOCK(mode) (((mode) & FS_FLAG_IFMT) == FS_FLAG_IFSOCK)

#define FS_FLAG_MOUNT   0x10    /* is mountpoint */

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
typedef struct fs_node *(*finddir_type_t)(struct fs_node *, const char *);
typedef ssize_t (*readlink_type_t)(struct fs_node *, char *, size_t);
typedef int (*truncate_type_t)(struct fs_node *, size_t);
typedef int (*mknod_type_t)(struct fs_node *, const char *, mode_t);
typedef int (*unlink_type_t)(struct fs_node *, const char *);
typedef int (*link_type_t)(struct fs_node *, const char *, struct fs_node *);
typedef int (*symlink_type_t)(struct fs_node *, const char *, const char *);
typedef int (*chmod_type_t)(struct fs_node *, mode_t);
typedef int (*chown_type_t)(struct fs_node *, uid_t, gid_t);
typedef int (*ioctl_type_t)(struct fs_node *, unsigned long, void *);

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
    truncate_type_t truncate;
    mknod_type_t mknod;
    unlink_type_t unlink;
    link_type_t link;
    symlink_type_t symlink;
    chmod_type_t chmod;
    chown_type_t chown;
    ioctl_type_t ioctl;

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

ssize_t fs_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
ssize_t fs_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
void fs_open(fs_node_t *node, unsigned flags);
void fs_close(fs_node_t *node);
struct dirent *fs_readdir(fs_node_t *node, off_t idx);
fs_node_t *fs_finddir(fs_node_t *node, const char *name);
int fs_readlink(fs_node_t *node, char *buf, size_t sz);
int fs_truncate(fs_node_t *node, size_t sz);
int fs_mknod(const char *path, mode_t mode);
int fs_unlink(const char *path);
int fs_link(const char *oldpath, const char *newpath);
int fs_symlink(const char *target, const char *path);
int fs_chmod(fs_node_t *node, mode_t mode);
int fs_chown(fs_node_t *node, uid_t uid, gid_t gid);
int fs_ioctl(fs_node_t *node, unsigned long request, void *argp);

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
