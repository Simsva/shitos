#ifndef KERNEL_PIPE_H_
#define KERNEL_PIPE_H_

#include <kernel/fs.h>

typedef struct pipe_device {
    uint8_t *buf;
    size_t write_ptr, read_ptr, sz;
} pipe_device_t;

size_t pipe_size(fs_node_t *node);
size_t pipe_avail(fs_node_t *node);

fs_node_t *pipe_create(size_t sz);
void pipe_destroy(fs_node_t *node);

#endif // KERNEL_PIPE_H_
