#include <kernel/pipe.h>
#include <kernel/kmem.h>
#include <string.h>

/**
 * Unidirectional buffer using VFS
 */

static ssize_t pipe_read(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
static ssize_t pipe_write(fs_node_t *node, off_t off, size_t sz, uint8_t *buf);
static inline size_t pipe_size_p(pipe_device_t *pipe);
static inline size_t pipe_avail_p(pipe_device_t *pipe);

static ssize_t pipe_read(fs_node_t *node, __unused off_t off, size_t sz, uint8_t *buf) {
    pipe_device_t *pipe = (pipe_device_t *)node->device;
    if(!pipe) return -1;

    size_t read = 0;
    while(pipe_size_p(pipe) > 0 && read < sz) {
        buf[read++] = pipe->buf[pipe->read_ptr];
        pipe->read_ptr = (pipe->read_ptr + 1) % pipe->sz;
    }

    return read;
}

static ssize_t pipe_write(fs_node_t *node, __unused off_t off, size_t sz, uint8_t *buf) {
    pipe_device_t *pipe = (pipe_device_t *)node->device;
    if(!pipe) return -1;

    size_t written = 0;
    while(pipe_avail_p(pipe) > 0 && written < sz) {
        pipe->buf[pipe->write_ptr] = buf[written++];
        pipe->write_ptr = (pipe->write_ptr + 1) % pipe->sz;
    }

    return written;
}

static inline size_t pipe_size_p(pipe_device_t *pipe) {
    return (pipe->read_ptr == pipe->write_ptr)
         ? 0
         : pipe->read_ptr > pipe->write_ptr
         ? (pipe->sz - pipe->read_ptr) + pipe->write_ptr
         : pipe->write_ptr - pipe->read_ptr;
}

/* TODO: a safer way to call this (to make sure node is a pipe) */
size_t pipe_size(fs_node_t *node) {
    return pipe_size_p((pipe_device_t *)node->device);
}

static inline size_t pipe_avail_p(pipe_device_t *pipe) {
    return (pipe->read_ptr == pipe->write_ptr)
         ? pipe->sz - 1
         : pipe->read_ptr > pipe->write_ptr
         ? pipe->read_ptr - pipe->write_ptr - 1
         : (pipe->sz - pipe->write_ptr) + pipe->read_ptr - 1;
}

size_t pipe_avail(fs_node_t *node) {
    return pipe_avail_p((pipe_device_t *)node->device);
}

void pipe_destroy(fs_node_t *node) {
    pipe_device_t *pipe = (pipe_device_t *)node->device;
    kfree(pipe->buf);
    kfree(pipe);
}

fs_node_t *pipe_create(size_t sz) {
    fs_node_t *fnode = kmalloc(sizeof(fs_node_t));
    pipe_device_t *pipe = kmalloc(sizeof(pipe_device_t));
    memset(fnode, 0, sizeof(fs_node_t));
    memset(pipe, 0, sizeof(pipe_device_t));

    fnode->device = pipe;
    strcpy(fnode->name, "[pipe]");
    fnode->uid = 0;
    fnode->gid = 0;
    fnode->mask = 0666;
    fnode->flags = FS_FLAG_IFIFO;
    fnode->read = pipe_read;
    fnode->write = pipe_write;

    pipe->buf = kmalloc(sz);
    pipe->sz = sz;

    return fnode;
}
