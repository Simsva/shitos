#include <kernel/video.h>
#include <kernel/args.h>
#include <kernel/fs.h>
#include <kernel/vmem.h>
#include <kernel/kmem.h>
#include <kernel/pci.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <kernel/arch/i386/ports.h>

#define PREFERRED_X 1280
#define PREFERRED_Y 720
#define PREFERRED_B 32

/* QEMU */
#define QEMU_MMIO_ID       0x00
#define QEMU_MMIO_FBWIDTH  0x02
#define QEMU_MMIO_FBHEIGHT 0x04
#define QEMU_MMIO_BPP      0x06
#define QEMU_MMIO_ENABLED  0x08
#define QEMU_MMIO_VIRTX    0x0c
#define QEMU_MMIO_VIRTY    0x0e

/* TODO: syscalls + signals, because now this does nothing */
#define validate(ptr) vmem_validate_user_ptr(ptr, 1, 0)

static fs_node_t *vga_text_dev, *fb_dev;

const char *fb_driver_name = NULL;
uint16_t fb_width = 0, fb_height = 0, fb_depth = 0, fb_stride = 0;
uint8_t *fb_vid_memory = (uint8_t *)0xe0000000;
size_t fb_memsize = 0;
static void (*fb_set_resolution_impl)(uint16_t w, uint16_t h) = NULL;

static void *qemu_mmio_mem = NULL;

static int vga_ioctl(fs_node_t *node, unsigned long request, void *argp);
static int fb_ioctl(fs_node_t *node, unsigned long request, void *argp);
static void create_fb_dev(const char *driver);

/* QEMU */
static void qemu_scan_pci(pci_device_t device, uint16_t vid, uint16_t did, void *extra);
static uint16_t qemu_mmio_in(unsigned off);
static void qemu_mmio_out(unsigned off, uint16_t val);
static void qemu_set_resolution(uint16_t w, uint16_t h);

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
            vmem_frame_map_addr(vmem_get_page(vga_user_off, VMEM_GET_CREATE),
                                VMEM_FLAG_WRITE, 0xb8000);
            *((uintptr_t *)argp) = vga_user_off;
        }
        return 0;
    default:
        return -EINVAL;
    }
}

static int fb_ioctl(__unused fs_node_t *node, __unused unsigned long request, __unused void *argp) {
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

static void qemu_scan_pci(pci_device_t device, uint16_t vid, uint16_t did, void *extra) {
    uintptr_t *out = extra;

    /* Bochs/QEMU VGA controller */
    if((vid == 0x1234 && did == 0x1111)) {
        /* video + mmio memory */
        uintptr_t t = pci_read_register(device, PCI_BAR0, 4);
        uintptr_t m = pci_read_register(device, PCI_BAR2, 4);

        if(m == 0) return;

        if(t > 0) {
            /* find size */
            pci_write_register(device, PCI_BAR0, 4, 0xffffffff);
            uint32_t s = pci_read_register(device, PCI_BAR0, 4);
            s = ~(s & -15) + 1;
            out[2] = s;
            pci_write_register(device, PCI_BAR0, 4, t);

            /* map video + mmio memory */
            out[0] = (uintptr_t)vmem_map_vaddr_n(t & 0xfffffff0, s);
            out[1] = (uintptr_t)vmem_map_vaddr(m & 0xfffffff0);
        }
    }
}

static uint16_t qemu_mmio_in(unsigned off) {
    return *(volatile uint16_t *)(qemu_mmio_mem + 0x500 + off);
}

static void qemu_mmio_out(unsigned off, uint16_t val) {
    *(volatile uint16_t *)(qemu_mmio_mem + 0x500 + off) = val;
}

static void qemu_set_resolution(uint16_t w, uint16_t h) {
    qemu_mmio_out(QEMU_MMIO_ENABLED, 0);
    qemu_mmio_out(QEMU_MMIO_FBWIDTH, w);
    qemu_mmio_out(QEMU_MMIO_FBHEIGHT, h);
    qemu_mmio_out(QEMU_MMIO_BPP, PREFERRED_B);
    qemu_mmio_out(QEMU_MMIO_VIRTX, w);
    qemu_mmio_out(QEMU_MMIO_VIRTY, h);
    /* 0x01: enabled, 0x40: linear framebuffer */
    qemu_mmio_out(QEMU_MMIO_ENABLED, 0x41);

    /* update fb_* variables */
    fb_width = qemu_mmio_in(QEMU_MMIO_FBWIDTH);
    fb_height = qemu_mmio_in(QEMU_MMIO_FBHEIGHT);
    fb_depth = qemu_mmio_in(QEMU_MMIO_BPP);
    fb_stride = qemu_mmio_in(QEMU_MMIO_VIRTX) * (fb_depth / 8);
}

static void graphics_install_text(void) {
    vmem_frame_map_addr(vmem_get_page((uintptr_t)fb_vid_memory, VMEM_GET_CREATE),
                        VMEM_FLAG_WRITE, 0xb8000);

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
    vmem_frame_map_addr(vmem_get_page((uintptr_t)fb_vid_memory, VMEM_GET_CREATE),
                        VMEM_FLAG_WRITE, kernel_args.video_memory);

    fb_stride = fb_width * fb_depth/8;
    fb_memsize = fb_stride * fb_height;

    create_fb_dev("preset");
}

static void graphics_install_qemu(void) {
    uintptr_t vals[3] = {0};
    pci_scan(qemu_scan_pci, -1, vals);

    if(!vals[0]) return;

    fb_vid_memory = (uint8_t *)vals[0];
    qemu_mmio_mem = (void *)vals[1];
    fb_memsize = vals[2];

    uint16_t id = qemu_mmio_in(QEMU_MMIO_ID);
    printf("QEMU BGA: id: %#06x\n", id);
    if(id < 0xb0c0 || id > 0xb0c6) return;

    qemu_set_resolution(fb_width, fb_height);
    fb_set_resolution_impl = qemu_set_resolution;

    if(!fb_vid_memory) {
        printf("QEMU: failed to find video memory\n");
        return;
    }

    create_fb_dev("qemu");
}

void fb_set_resolution(uint16_t w, uint16_t h) {
    if(fb_set_resolution_impl)
        fb_set_resolution_impl(w, h);
}

void fb_init(void) {
    fb_width = kernel_args.video_x ? kernel_args.video_x : PREFERRED_X;
    fb_height = kernel_args.video_y ? kernel_args.video_y : PREFERRED_Y;
    fb_depth = kernel_args.video_depth ? kernel_args.video_depth : PREFERRED_B;
    /* stride will be calculated later */

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
