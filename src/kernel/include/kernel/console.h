#ifndef KERNEL_CONSOLE_H_
#define KERNEL_CONSOLE_H_

#include <kernel/fs.h>
#include <stdint.h>
#include <stddef.h>

extern fs_node_t *console_dev;

void console_set_output(ssize_t (*output)(size_t, uint8_t *));
void console_install(void);

#endif // KERNEL_CONSOLE_H_
