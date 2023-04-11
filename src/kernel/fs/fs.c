#include <kernel/fs.h>

#include <kernel/hashmap.h>
#include <kernel/kmem.h>
#include <sys/stat.h>
#include <features.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_SYMLINK_DEPTH 8
#define MAX_SYMLINK_SIZE 0x1000

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define TREE_TO_FS_NODE(tree_node) (((struct vfs_entry *)((tree_node)->value))->file)

tree_t *fs_tree = NULL;
static hashmap_t *fs_types = NULL;

static void get_canon_parent_basename(const char *cwd, const char *path, size_t *pathsz, char **canon, char **parent, char **basename);

/**
 * Write the canonicalized path to canon, parent directory to parent, and the
 * basename to basename. Canon and parent are freed by the caller, basename is
 * simply an offset in canon.
 * If path is the root directory, sets parent and basename to NULL.
 *
 * XXX: does no any sanity checks at all on path
 */
static void get_canon_parent_basename(const char *cwd, const char *path, size_t *pathsz, char **canon, char **parent, char **basename) {
    char *b;
    *canon = canonicalize_path(cwd, path, pathsz);

    if(*pathsz < 2) {
        /* path is "/" */
        *parent = *basename = NULL;
        return;
    }

    for(b = *canon + *pathsz-1; *b != PATH_SEPARATOR; b--);
    if(*canon == b) {
        /* if first PATH_SEPARATOR was the "root slash" */
        *parent = strdup("/");
        *basename = b+1;
    } else {
        /* memcpy parent part of the path */
        size_t parentsz = b - *canon;
        *parent = kmalloc(parentsz + 1);
        memcpy(*parent, *canon, parentsz);
        (*parent)[parentsz] = '\0';

        *basename = b+1;
    }
}

ssize_t fs_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    if(!node) return -ENOENT;
    if(node->read) return node->read(node, off, sz, buf);
    else return -EINVAL;
}

ssize_t fs_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf) {
    if(!node) return -ENOENT;
    if(node->write) return node->write(node, off, sz, buf);
    else return -EINVAL;
}

void fs_open(fs_node_t *node, unsigned flags) {
    if(!node) return;
    if(node->refcount >= 0) node->refcount++;
    if(node->open) node->open(node, flags);
}

void fs_close(fs_node_t *node) {
    if(!node) return;
    /* never close files with negative refcounts */
    if(node->refcount < 0) return;

    /* decrement refcount, if zero free file */
    if(--node->refcount != 0) return;

    if(node->close) node->close(node);
    kfree(node);
}

struct dirent *fs_readdir(fs_node_t *node, off_t idx) {
    if(!node) return NULL;
    if(node->readdir && FS_ISDIR(node->flags))
        return node->readdir(node, idx);
    else return NULL;
}

fs_node_t *fs_finddir(fs_node_t *node, const char *name) {
    if(!node) return NULL;
    if(node->finddir && FS_ISDIR(node->flags))
        return node->finddir(node, name);
    else return NULL;
}

int fs_readlink(fs_node_t *node, char *buf, size_t sz) {
    if(!node) return -ENOENT;
    if(node->readlink) return node->readlink(node, buf, sz);
    else return -EINVAL;
}

int fs_truncate(fs_node_t *node, size_t sz) {
    if(!node) return -ENOENT;
    if(node->truncate) return node->truncate(node, sz);
    else return -EINVAL;
}

int fs_mknod(const char *path, mode_t mode) {
    if(!path) return -EFAULT;
    if(!path[0]) return -ENOENT;

    /* make sure the type is valid */
    /* links should be created with fs_symlink */
    switch(mode & S_IFMT) {
    case S_IFDIR: case S_IFCHR: case S_IFBLK:
    case S_IFREG: case S_IFIFO: case S_IFSOCK:
        break;
    default: return -EINVAL;
    }

    size_t pathsz;
    char *cpath, *basename, *parent;
    int ret = 0;

    get_canon_parent_basename("/", path, &pathsz, &cpath, &parent, &basename);

    if(!parent || !basename) {
        /* root directory */
        kfree(cpath);
        return -EEXIST;
    }

    if(strlen(basename) > 255) {
        ret = -ENAMETOOLONG;
        goto ret_free;
    }

    /* test if path already exists */
    fs_node_t *tmp = kopen(cpath, 0);
    if(tmp) {
        fs_close(tmp);
        ret = -EEXIST;
        goto ret_free;
    }

    fs_node_t *fparent = kopen(parent, 0);
    if(!fparent) {
        ret = -ENOENT;
        goto ret_free;
    }
    if(!FS_ISDIR(fparent->flags)) {
        ret = -ENOTDIR;
        fs_close(fparent);
        goto ret_free;
    }

    /* TODO: check permissions? */

    ret = fparent->mknod
        ? fparent->mknod(fparent, basename, mode)
        : -EROFS;
    fs_close(fparent);

ret_free:
    kfree(cpath);
    kfree(parent);
    return ret;
}

int fs_unlink(const char *path) {
    if(!path) return -EFAULT;
    if(!path[0]) return -ENOENT;

    size_t pathsz;
    char *cpath, *basename, *parent;
    int ret = 0;

    get_canon_parent_basename("/", path, &pathsz, &cpath, &parent, &basename);

    if(!parent || !basename) {
        /* root directory */
        kfree(cpath);
        return -EBUSY;
    }

    if(strlen(basename) > 255) {
        ret = -ENAMETOOLONG;
        goto ret_free;
    }

    /* test if path actually exists */
    fs_node_t *tmp = kopen(cpath, 0);
    if(!tmp) {
        ret = -ENOENT;
        goto ret_free;
    }
    fs_close(tmp);

    fs_node_t *fparent = kopen(parent, 0);
    if(!fparent) {
        ret = -ENOENT;
        goto ret_free;
    }
    if(!FS_ISDIR(fparent->flags)) {
        ret = -ENOTDIR;
        fs_close(fparent);
        goto ret_free;
    }

    /* TODO: check permissions? */

    ret = fparent->unlink
        ? fparent->unlink(fparent, basename)
        : -EROFS;
    fs_close(fparent);

ret_free:
    kfree(cpath);
    kfree(parent);
    return ret;
}

int fs_link(const char *oldpath, const char *newpath) {
    if(!oldpath || !newpath) return -EFAULT;
    if(!oldpath[0] || !newpath[0]) return -ENOENT;

    size_t pathsz;
    char *cpath, *basename, *parent;
    int ret = 0;

    /* follow symlinks in oldpath */
    fs_node_t *old = kopen(oldpath, 0);
    if(!old) return -ENOENT;
    if(FS_ISDIR(old->flags)) return -EPERM;

    get_canon_parent_basename("/", newpath, &pathsz, &cpath, &parent, &basename);

    if(!parent || !basename) {
        /* root directory */
        kfree(cpath);
        return -EBUSY;
    }

    if(strlen(basename) > 255) {
        ret = -ENAMETOOLONG;
        goto ret_free;
    }

    /* test if path already exists */
    fs_node_t *tmp = kopen(cpath, 0);
    if(tmp) {
        ret = -EEXIST;
        fs_close(tmp);
        goto ret_free;
    }

    fs_node_t *fparent = kopen(parent, 0);
    if(!fparent) {
        ret = -ENOENT;
        goto ret_free;
    }
    if(!FS_ISDIR(fparent->flags)) {
        ret = -ENOTDIR;
        fs_close(fparent);
        goto ret_free;
    }

    /* TODO: check permissions? */

    ret = fparent->link
        ? fparent->link(fparent, basename, old)
        : -EROFS;
    fs_close(fparent);

ret_free:
    fs_close(old);
    kfree(cpath);
    kfree(parent);
    return ret;
}

int fs_symlink(const char *target, const char *path) {
    if(!target || !path) return -EFAULT;
    if(!target[0] || !path[0]) return -ENOENT;

    size_t pathsz;
    char *cpath, *basename, *parent;
    int ret = 0;

    get_canon_parent_basename("/", path, &pathsz, &cpath, &parent, &basename);

    if(!parent || !basename) {
        /* root directory */
        kfree(cpath);
        return -EBUSY;
    }

    if(strlen(basename) > 255) {
        ret = -ENAMETOOLONG;
        goto ret_free;
    }

    /* test if path already exists */
    fs_node_t *tmp = kopen(cpath, 0);
    if(tmp) {
        ret = -EEXIST;
        fs_close(tmp);
        goto ret_free;
    }

    fs_node_t *fparent = kopen(parent, 0);
    if(!fparent) {
        ret = -ENOENT;
        goto ret_free;
    }
    if(!FS_ISDIR(fparent->flags)) {
        ret = -ENOTDIR;
        fs_close(fparent);
        goto ret_free;
    }

    /* TODO: check permissions? */

    ret = fparent->symlink
        ? fparent->symlink(fparent, basename, target)
        : -EROFS;
    fs_close(fparent);

ret_free:
    kfree(cpath);
    kfree(parent);
    return ret;
}

int fs_chmod(fs_node_t *node, mode_t mode) {
    if(!node) return -ENOENT;
    if(node->chmod) return node->chmod(node, mode & 07777);
    return -EROFS;
}

int fs_chown(fs_node_t *node, uid_t uid, gid_t gid) {
    if(!node) return -ENOENT;
    if(node->chown) return node->chown(node, uid, gid);
    return -EROFS;
}

int fs_ioctl(fs_node_t *node, unsigned long request, void *argp) {
    if(!node) return -ENOENT;
    if(node->ioctl) return node->ioctl(node, request, argp);
    return -EINVAL;
}

void vfs_install(void) {
    fs_tree = tree_create();
    fs_types = hashmap_create_str(5);
    fs_types->value_free = NULL;

    struct vfs_entry *root = kmalloc(sizeof(struct vfs_entry));
    root->name = strdup("[root]");
    root->file = NULL;
    root->fs_type = NULL;
    root->args = NULL;

    tree_set_root(fs_tree, root);
}

static struct dirent *mapper_readdir(fs_node_t *node, off_t idx) {
    tree_node_t *d = (tree_node_t *)node->device;

    if(!d) return NULL;

    if(idx == 0) {
        /* . */
        struct dirent *dir = kmalloc(sizeof(struct dirent));
        strcpy(dir->d_name, PATH_CURRENT);
        dir->d_ino = 0;
        return dir;
    } else if(idx == 1) {
        /* .. */
        struct dirent *dir = kmalloc(sizeof(struct dirent));
        strcpy(dir->d_name, PATH_GO_UP);
        dir->d_ino = 1;
        return dir;
    }

    idx -= 2;
    off_t i = 0;
    list_foreach(child, d->children) {
        if(i == idx) {
            tree_node_t *tchild = (tree_node_t *)child->value;
            struct vfs_entry *entry = (struct vfs_entry *)tchild->value;
            struct dirent *dir = kmalloc(sizeof(struct dirent));

            size_t l = strlen(entry->name) + 1;
            memcpy(&dir->d_name, entry->name, MIN(256, l));
            dir->d_ino = i;
            return dir;
        }
        i++;
    }

    return NULL;
}

static fs_node_t *vfs_mapper(void) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    memset(fnode, 0, sizeof(fs_node_t));
    strcpy(fnode->name, "mapped");
    fnode->mask = 0555;
    fnode->flags = FS_FLAG_IFDIR;
    fnode->readdir = mapper_readdir;
    return fnode;
}

int vfs_register_type(const char *type, vfs_mount_t mount) {
    if(hashmap_has(fs_types, type)) return 1;
    hashmap_set(fs_types, type, mount);
    return 0;
}

void *vfs_mount(const char *path, fs_node_t *local_root) {
    if(!fs_tree) {
        errno = ENOENT;
        return NULL;
    }
    if(!path || path[0] != PATH_SEPARATOR) {
        errno = EINVAL;
        return NULL;
    }

    local_root->refcount = -1;

    tree_node_t *retval = NULL;
    char *p = strdup(path), *head = p;
    size_t l = strlen(p);

    /* chop the path up */
    for(; head < p + l; head++)
        if(*head == PATH_SEPARATOR) *head = '\0';
    p[l] = '\0';
    head = p + 1;

    tree_node_t *root_node = fs_tree->root;

    if(*head == '\0') {
        /* setting the root node */
        struct vfs_entry *root = (struct vfs_entry *)root_node->value;
        if(root->file) {
            /* TODO: kerror warning already mounted */
            printf("WARN: root is already mounted, remounting\n");
        }
        root->file = local_root;
        retval = root_node;
    } else {
        tree_node_t *node = root_node;

        while(head < p + l) {
            int found = 0;

            list_foreach(child, node->children) {
                /* searching for *head */
                tree_node_t *tchild = (tree_node_t *)child->value;
                struct vfs_entry *entry = (struct vfs_entry *)tchild->value;
                if(!strcmp(entry->name, head)) {
                    found = 1;
                    node = tchild;
                    retval = node;
                    break;
                }
            }
            if(!found) {
                /* not found => create new */
                struct vfs_entry *entry = kmalloc(sizeof(struct vfs_entry));
                entry->name = strdup(head);
                entry->file = NULL;
                entry->args = NULL;
                entry->fs_type = NULL;
                node = tree_insert_item(fs_tree, node, entry);
            }
            head += strlen(head) + 1;
        }

        struct vfs_entry *entry = (struct vfs_entry *)node->value;
        if(entry->file) {
            /* TODO: kerror warning already mounted */
            printf("WARN: %s is already mounted, remounting\n", path);
        }
        entry->file = local_root;
        retval = node;
    }

    kfree(p);
    return retval;
}

int vfs_mount_type(const char *type, const char *arg, const char *mountpoint) {
    vfs_mount_t mount = hashmap_get(fs_types, type);
    if(!mount) {
        /* Unknown filesystem */
        return -ENODEV;
    }

    fs_node_t *node = mount(arg, mountpoint);

    /* HACK: let partition mappers not return a node to mount */
    if((uintptr_t)node == 1) return 0;
    if(!node) return -EINVAL;

    tree_node_t *tnode = vfs_mount(mountpoint, node);
    if(tnode && tnode->value) {
        struct vfs_entry *entry = tnode->value;
        entry->fs_type = strdup(type);
        entry->args = strdup(arg);
    }

    return 0;
}

void vfs_map_directory(const char *path) {
    fs_node_t *f = vfs_mapper();
    struct vfs_entry *entry = vfs_mount(path, f);
    f->device = entry;
}

/* canonicalize a path relative to cwd
 * if length is not NULL, the size of the path string will be written to it */
char *canonicalize_path(const char *cwd, const char *path, size_t *length) {
    /* output stack */
    list_t *out = list_create();

    /* if path is relative we need to canonicalize cwd
     * and push it to the stack */
    if(path[0] != '\0' && path[0] != PATH_SEPARATOR) {
        char *cwd2 = strdup(cwd);

        char *pch, *save;
        pch = strtok_r(cwd2, PATH_SEPARATOR_STR, &save);

        while(pch != NULL) {
            char *s = strdup(pch);
            list_push_item(out, s);
            pch = strtok_r(NULL, PATH_SEPARATOR_STR, &save);
        }
        kfree(cwd2);
    }

    /* push path (and parse . and ..) */
    char *path2 = strdup(path);

    char *pch, *save;
    pch = strtok_r(path2, PATH_SEPARATOR_STR, &save);

    while(pch != NULL) {
        if(!strcmp(pch, PATH_CURRENT)) {
            /* path = . => do nothing */
        } else if(!strcmp(pch, PATH_GO_UP)) {
            /* path = .. => pop the stack */
            list_node_t *node = list_pop(out);
            if(node) {
                kfree(node->value);
                kfree(node);
            }
        } else {
            /* regular path => push it */
            char *s = strdup(pch);
            list_push_item(out, s);
        }
        pch = strtok_r(NULL, PATH_SEPARATOR_STR, &save);
    }
    kfree(path2);

    /* if the stack is empty, return "/" */
    if(out->sz == 0) {
        list_free(out);
        if(length) *length = 1;
        return strdup("/");
    }

    /* size of path string */
    size_t sz = 0;
    list_foreach(node, out) sz += strlen(node->value) + 1;

    /* join the list */
    char *outstr = kmalloc(sz + 1), *head = outstr;
    list_foreach(node, out) {
        (head++)[0] = PATH_SEPARATOR;
        size_t nodesz = strlen(node->value);
        memcpy(head, node->value, nodesz + 1);
        head += nodesz;
    }

    list_destroy(out);
    list_free(out);
    if(length) *length = sz;
    return outstr;
}

static fs_node_t *get_mount_point(char *path, size_t path_depth, char **outpath, size_t *outdepth) {
    /* point path at the end of the path */
    path++;
    for(size_t depth = 0; depth < path_depth; depth++)
        path += strlen(path) + 1;

    fs_node_t *last = TREE_TO_FS_NODE(fs_tree->root);
    tree_node_t *node = fs_tree->root;

    char *at = *outpath;
    int _depth = 1;
    int _tree_depth = 0;

    /* traverse the tree until we find a leaf (or the path ends) */
    while(at < path) {
        int found = 0;

        list_foreach(child, node->children) {
            tree_node_t *tchild = (tree_node_t *)child->value;
            struct vfs_entry *entry = (struct vfs_entry *)tchild->value;
            if(!strcmp(entry->name, at)) {
                found = 1;
                node = tchild;
                at += strlen(at) + 1;
                if(entry->file) {
                    _tree_depth = _depth;
                    last = entry->file;
                    *outpath = at;
                }
                break;
            }
        }
        if(!found) break;
        _depth++;
    }

    *outdepth = _tree_depth;

    if(last) {
        fs_node_t *last_clone = kmalloc(sizeof(fs_node_t));
        memcpy(last_clone, last, sizeof(fs_node_t));
        last_clone->refcount = 0;
        return last_clone;
    }
    return NULL;
}

/* assumes root is mounted, can dereference a NULL pointer otherwise */
static fs_node_t *kopen_recur(const char *file, unsigned flags, unsigned symlink_depth, const char *cwd) {
    if(!file) {
        errno = EINVAL;
        return NULL;
    }

    size_t path_sz;
    char *path = canonicalize_path(cwd, file, &path_sz);

    /* if length is 1 then path = "/" */
    if(path_sz == 1) {
        fs_node_t *root_clone = kmalloc(sizeof(fs_node_t));
        memcpy(root_clone, TREE_TO_FS_NODE(fs_tree->root), sizeof(fs_node_t));
        root_clone->refcount = 0;

        kfree(path);
        /* open and return the clone */
        fs_open(root_clone, flags);
        return root_clone;
    }

    /* otherwise, chup the path up and start searching */
    char *path_head = path;
    size_t path_depth = 0;
    while(path_head < path + path_sz) {
        if(*path_head == PATH_SEPARATOR)
            *path_head = '\0', path_depth++;
        path_head++;
    }
    path_head = path+1;

    /* path_head points to the first directory and path_depth is the number of
     * directories, traverse fs_tree to find the file */

    size_t depth = 0;
    /* traverse the tree to the first leaf
     * either the file we are looking for, a symlink, or a mount point */
    fs_node_t *node = get_mount_point(path, path_depth, &path_head, &depth);

    if(!node) {
        errno = ENOENT;
        return NULL;
    }

    do {
        /*
         * Always follow symlinks in the middle of a path even if O_NOFOLLOW and
         * O_PATH are set. However, if they are set we should not follow a
         * symlink if it is the "leaf" of the path.
         */
         if(FS_ISLNK(node->flags) &&
            !((flags & O_NOFOLLOW) && (flags & O_PATH) && depth == path_depth)) {
             if((flags & O_NOFOLLOW) && depth == path_depth - 1) {
                 /* do not follow the final entry with O_NOFOLLOW */
                 errno = ELOOP;
                 kfree(path);
                 kfree(node);
                 return NULL;
             }
             if(symlink_depth >= MAX_SYMLINK_DEPTH) {
                 errno = ELOOP;
                 kfree(path);
                 kfree(node);
                 return NULL;
             }

             /* NOTE: maybe heap allocate this large buffer in a recursive
              * function */
             char symlink_buf[MAX_SYMLINK_SIZE];
             int len = fs_readlink(node, symlink_buf, sizeof symlink_buf);
             if(len < 0) {
                 errno = -len;
                 kfree(path);
                 kfree(node);
                 return NULL;
             }
             if(symlink_buf[len] != '\0') {
                 errno = ENAMETOOLONG; /* feels weird */
                 kfree(path);
                 kfree(node);
                 return NULL;
             }

             kfree(node);

             /* rebuild the path up to this point to
              * use as a relative path for the symlink */
             char *relpath = kmalloc(path_sz + 1);
             char *head = relpath;
             memcpy(relpath, path, path_sz + 1);
             for(size_t i = 0; depth && i < depth - 1; i++) {
                 while(*head++ != '\0');
                 *head = PATH_SEPARATOR;
             }
             node = kopen_recur(symlink_buf, 0, symlink_depth + 1, relpath);
             kfree(relpath);

             if(!node) {
                 errno = ENOENT;
                 kfree(path);
                 return NULL;
             }
         }
         if(path_head >= path + path_sz
            || depth == path_depth) {
             /* found the file */
             fs_open(node, flags);
             kfree(path);
             return node;
         }

         /* if not found, search the active directory */
         fs_node_t *node_next = fs_finddir(node, path_head);
         kfree(node);
         node = node_next;

         if(!node) {
             errno = ENOENT;
             kfree(path);
             return NULL;
         }
         path_head += strlen(path_head) + 1;
         depth++;
    } while(depth < path_depth + 1);

    /* failed to find the requested file */
    errno = ENOENT;
    kfree(path);
    return NULL;
}

/* like open(2), but returns an fs_node instead of a fd */
fs_node_t *kopen(const char *path, unsigned flags) {
    /* TODO: cwd that is not hardcoded to "/" maybe? */
    return kopen_recur(path, flags, 0, "/");
}
