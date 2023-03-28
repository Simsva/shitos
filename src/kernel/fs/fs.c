#include <kernel/fs.h>

#include <features.h>
#include <fcntl.h>
#include <kernel/kmem.h>
#include <string.h>

#define MAX_SYMLINK_DEPTH 8
#define MAX_SYMLINK_SIZE 0x1000

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define TREE_TO_FS_NODE(tree_node) (((struct vfs_entry *)((tree_node)->value))->file)

tree_t *fs_tree = NULL;

ssize_t fs_read(struct fs_node *node, off_t off, size_t sz, uint8_t *buf) {
    if(!node) return -1;
    if(node->read) return node->read(node, off, sz, buf);
    else return -1;
}

ssize_t fs_write(struct fs_node *node, off_t off, size_t sz, uint8_t *buf) {
    if(!node) return -1;
    if(node->write) return node->write(node, off, sz, buf);
    else return -1;
}

void fs_open(struct fs_node *node, unsigned flags) {
    if(!node) return;
    if(node->open) node->open(node, flags);
}

void fs_close(struct fs_node *node) {
    if(!node) return;
    if(node->close) node->close(node);
}

struct dirent *fs_readdir(struct fs_node *node, off_t idx) {
    if(!node) return NULL;
    if(node->readdir && (node->flags & FS_FLAG_TYPE_MASK) == FS_TYPE_DIR)
        return node->readdir(node, idx);
    else return NULL;
}

fs_node_t *fs_finddir(fs_node_t *node, char *name) {
    if(!node) return NULL;
    if(node->finddir && (node->flags & FS_FLAG_TYPE_MASK) == FS_TYPE_DIR)
        return node->finddir(node, name);
    else return NULL;
}

int fs_readlink(fs_node_t *node, char *buf, size_t sz) {
    if(!node) return -1;
    if(node->readlink) return node->readlink(node, buf, sz);
    else return -1;
}

void vfs_install(void) {
    fs_tree = tree_create();

    struct vfs_entry *root = kmalloc(sizeof(struct vfs_entry));
    root->name = strdup("[root]");
    root->file = NULL;
    root->fs_type = NULL;
    root->args = NULL;

    tree_set_root(fs_tree, root);
}

static struct dirent *readdir_mapper(fs_node_t *node, off_t idx) {
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
    fnode->mask = 0555;
    fnode->flags = FS_TYPE_DIR;
    fnode->readdir = readdir_mapper;
    return fnode;
}

void *vfs_mount(const char *path, fs_node_t *local_root) {
    if(!fs_tree) {
        /* TODO: kerror */
        return NULL;
    }
    if(!path || path[0] != PATH_SEPARATOR) {
        /* TODO: kerror */
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
        /* if(root->file) { */
            /* TODO: kerror warning already mounted */
        /* } */
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
        /* if(entry->file) { */
            /* TODO: kerror warning already mounted */
        /* } */
        entry->file = local_root;
        retval = node;
    }

    kfree(p);
    return retval;
}

void vfs_map_directory(const char *path) {
    fs_node_t *f = vfs_mapper();
    struct vfs_entry *entry = vfs_mount(path, f);
    if(!strcmp(path, "/"))
        f->device = fs_tree->root;
    else
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
    if(!file) return NULL;

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

    if(!node) return NULL;

    do {
        /*
         * Always follow symlinks in the middle of a path even if O_NOFOLLOW and
         * O_PATH are set. However, if they are set we should not follow a
         * symlink if it is the "leaf" of the path.
         */
         if((node->flags & FS_FLAG_TYPE_MASK) == FS_TYPE_LINK &&
            !((flags & O_NOFOLLOW) && (flags & O_PATH) && depth == path_depth)) {
             if((flags & O_NOFOLLOW) && depth == path_depth - 1) {
                 /* TODO: ELOOP */
                 /* do not follow the final entry with O_NOFOLLOW */
                 kfree(path);
                 kfree(node);
                 return NULL;
             }
             if(symlink_depth >= MAX_SYMLINK_DEPTH) {
                 /* TODO: ELOOP */
                 kfree(path);
                 kfree(node);
                 return NULL;
             }

             /* NOTE: maybe heap allocate this large buffer in a recursive
              * function */
             char symlink_buf[MAX_SYMLINK_SIZE];
             int len = fs_readlink(node, symlink_buf, sizeof symlink_buf);
             if(len < 0) {
                 /* TODO: errno */
                 kfree(path);
                 kfree(node);
                 return NULL;
             }
             if(symlink_buf[len] != '\0') {
                 /* TODO: errno */
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
                 /* failed to follow symlink, dangling symlink? */
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
             kfree(path);
             return NULL;
         }
         path_head += strlen(path_head) + 1;
         depth++;
    } while(depth < path_depth + 1);

    /* failed to find the requested file */
    kfree(path);
    return NULL;
}

/* like open(2), but returns an fs_node instead of a fd */
fs_node_t *kopen(const char *path, unsigned flags) {
    /* TODO: cwd that is not hardcoded to "/" maybe? */
    return kopen_recur(path, flags, 0, "/");
}
