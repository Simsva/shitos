/* only for use in GDB */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

struct disk_packet {
    uint8_t size, reserved;
    uint16_t num_blocks, offset, segment;
    uint64_t lba;
} __attribute__((packed)) *_1 __attribute__((used)) = (void *)0xffffffff;

/* use `p_v86_rm_stack 0x5e00` */
struct v86_rm_stack {
    uint32_t rm_addr_target;
    uint16_t rm_flags_target;
    uint32_t rm_addr_tramp;
    uint32_t rm_flags_tramp;
    uint32_t _v86_ptr;
    uint32_t kernel_esp;
} __attribute__((packed)) *_2 __attribute__((used)) = (void *)0xffffffff;

/* use `p_v86_kernel_stack $esi` */
struct v86_kernel_stack {
    uint32_t es, ds, fs, gs;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
    uint32_t _v86_ptr;
} __attribute__((packed)) *_3 __attribute__((used)) = (void *)0xffffffff;

/* use `p_v86_rret_tramp_stack 0x5e00` */
struct v86_rret_tramp_stack {
    uint32_t eflags;
    uint32_t es, ds, fs, gs;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t _v86_ptr;
    uint32_t kernel_esp;
} __attribute__((packed)) *_4 __attribute__((used)) = (void *)0xffffffff;
