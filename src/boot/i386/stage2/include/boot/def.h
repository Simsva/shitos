#ifndef BOOT_DEF_H_
#define BOOT_DEF_H_

#include <stdint.h>

enum video_mode {
VIDEO_PRESET,
VIDEO_TEXT,
VIDEO_QEMU,
};

struct kernel_args {
    uint16_t tm_cursor;
    uint8_t boot_options;
    uint8_t drive_num;
    uint16_t video_x, video_y;
    uintptr_t video_memory;
    enum video_mode video_mode;
    uint8_t video_depth;
};

#endif // BOOT_DEF_H_
