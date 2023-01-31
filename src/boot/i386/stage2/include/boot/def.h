#ifndef BOOT_DEF_H_
#define BOOT_DEF_H_

#include <stdint.h>

struct kernel_args {
    uint16_t tm_cursor;
    uint8_t boot_options;
    uint8_t drive_num;
};

#endif // BOOT_DEF_H_
