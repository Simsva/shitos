#ifndef KERNEL_DEF_H_
#define KERNEL_DEF_H_

#include <sys/stdint.h>

struct kernel_args {
    uint16_t tm_cursor;
    uint8_t boot_options;
    uint8_t drive_num;
};

#endif // KERNEL_DEF_H_
