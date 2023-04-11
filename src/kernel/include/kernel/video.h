#ifndef KERNEL_VIDEO_H_
#define KERNEL_VIDEO_H_

#include <stdint.h>
#include <stddef.h>

#define IOCTL_VID_WIDTH  0x5001
#define IOCTL_VID_HEIGHT 0x5002
#define IOCTL_VID_MAP    0x5003
#define IOCTL_VID_DEPTH  0x5004
#define IOCTL_VID_STRIDE 0x5005
#define IOCTL_VID_SET    0x5006

#define VGA_TEXT_WIDTH   80
#define VGA_TEXT_HEIGHT  25

extern const char *fb_driver_name = NULL;
extern uint16_t fb_width = 0, fb_height = 0, fb_depth = 0, fb_stride = 0;
extern uint8_t *fb_vid_memory = (uint8_t *)0xe0000000;
extern size_t fb_memsize = 0;

struct video_size {
    uint16_t w, h;
};

#ifdef _KERNEL
void fb_init(void);
#endif

#endif // KERNEL_VIDEO_H_
