#include <kernel/video.h>
#include <kernel/args.h>
#include <kernel/fs.h>
#include <kernel/vmem.h>
#include <kernel/kmem.h>
#include <string.h>
#include <errno.h>

#define PREFERRED_X 1280
#define PREFERRED_Y 720

/* TODO: syscalls + signals, because now this does nothing */
#define validate(ptr) vmem_validate_ptr(ptr, 1, 0)

static fs_node_t *vga_text_dev, *fb_dev;

const char *fb_driver_name = NULL;
uint16_t fb_width = 0, fb_height = 0, fb_depth = 0, fb_stride = 0;
uint8_t *fb_vid_memory = (uint8_t *)0xe0000000;
size_t fb_memsize = 0;

static int vga_ioctl(fs_node_t *node, unsigned long request, void *argp);
static int fb_ioctl(fs_node_t *node, unsigned long request, void *argp);
static void create_fb_dev(const char *driver);
static void graphics_install_text(void);
static void graphics_install_preset(void);
static void graphics_install_qemu(void);

static int vga_ioctl(__unused fs_node_t *node, unsigned long request, void *argp) {
    switch(request) {
    case IOCTL_VID_WIDTH: /* get "framebuffer" width */
        validate(argp);
        *((size_t *)argp) = VGA_TEXT_WIDTH;
        return 0;
    case IOCTL_VID_HEIGHT: /* get "framebuffer" height */
        validate(argp);
        *((size_t *)argp) = VGA_TEXT_HEIGHT;
        return 0;
    case IOCTL_VID_MAP: /* map "framebuffer" in userspace memory */
        validate(argp);
        {
            uintptr_t vga_user_off;
            if(*(uintptr_t *)argp == 0) {
                return -EINVAL;
            } else {
                validate((void *)(*(uintptr_t *)argp));
                vga_user_off = *(uintptr_t *)argp;
            }
            vmem_map((void *)vga_user_off, (void *)0xb8000, PAGE_FLAG_RW);
            *((uintptr_t *)argp) = vga_user_off;
        }
        return 0;
    default:
        return -EINVAL;
    }
}

static int fb_ioctl(__unused fs_node_t *node, unsigned long request, void *argp) {
    return -EINVAL;
}

static void create_fb_dev(const char *driver) {
    fb_driver_name = driver;
    fb_dev = kmalloc(sizeof(fs_node_t));
    memset(fb_dev, 0, sizeof(fs_node_t));
    strcpy(fb_dev->name, "fb0");
    fb_dev->sz = fb_stride * fb_height;
    fb_dev->flags = FS_FLAG_IFBLK;
    fb_dev->mask = 0660;
    fb_dev->ioctl = fb_ioctl;
    vfs_mount("/dev/fb0", fb_dev);
}

static void graphics_install_text(void) {
    vmem_map((void *)0xb8000, fb_vid_memory, PAGE_FLAG_RW);

    vga_text_dev = kmalloc(sizeof(fs_node_t));
    memset(vga_text_dev, 0, sizeof(fs_node_t));
    strcpy(vga_text_dev->name, "vga0");
    vga_text_dev->sz = 0;
    vga_text_dev->flags = FS_FLAG_IFBLK;
    vga_text_dev->mask = 0660;
    vga_text_dev->ioctl = vga_ioctl;
    vfs_mount("/dev/vga0", vga_text_dev);
}

static void graphics_install_preset(void) {
    vmem_map((void *)kernel_args.video_memory, fb_vid_memory, PAGE_FLAG_RW);

    fb_stride = fb_width * fb_depth/8;
    fb_memsize = fb_stride * fb_height;

    create_fb_dev("preset");
}

static void graphics_install_qemu(void) {

}

void fb_init(void) {
    uint16_t x = 0, y = 0;
    uint8_t b = 0;
    if(kernel_args.video_x)
        x = kernel_args.video_x;
    if(kernel_args.video_y)
        y = kernel_args.video_y;
    if(kernel_args.video_depth)
        b = kernel_args.video_depth;

    fb_width = x, fb_height = y, fb_depth = b;

    switch(kernel_args.video_mode) {
    case VIDEO_TEXT:
        graphics_install_text();
        return;

    case VIDEO_PRESET:
        graphics_install_preset();
        return;

    case VIDEO_QEMU:
        graphics_install_qemu();
        return;
    }

    return;
}
