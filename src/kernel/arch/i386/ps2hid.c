#include <kernel/fs.h>
#include <kernel/pipe.h>
#include <kernel/arch/i386/ports.h>
#include <kernel/arch/i386/irq.h>

#include <stdio.h>

#define KEYBOARD_IRQ          1
#define MOUSE_IRQ            12

#define PS2_DATA           0x60
#define PS2_STATUS         0x64
#define PS2_COMMAND        0x64

#define PS2_PORT1_IRQ      0x01
#define PS2_PORT2_IRQ      0x02
#define PS2_PORT1_TLATE    0x40

#define PS2_READ_CONFIG    0x20
#define PS2_WRITE_CONFIG   0x60

#define PS2_DISABLE_PORT2  0xA7
#define PS2_ENABLE_PORT2   0xA8
#define PS2_DISABLE_PORT1  0xAD
#define PS2_ENABLE_PORT1   0xAE

#define KBD_SET_SCANCODE   0xF0

#define BIT(n) (1<<(n))

static fs_node_t *kbd_pipe = NULL;

static inline int ps2_wait_input(void) {
    uint32_t timeout = UINT32_C(100000);
    while(--timeout)
        if(!(inportb(PS2_STATUS) & BIT(1))) return 0;
    return 1;
}

static inline int ps2_wait_output(void) {
    uint32_t timeout = UINT32_C(100000);
    while(--timeout)
        if(inportb(PS2_STATUS) & BIT(0)) return 0;
    return 1;
}

static inline void ps2_cmd(uint8_t cmd) {
    ps2_wait_input();
    outportb(PS2_COMMAND, cmd);
}

static inline void ps2_cmd_arg(uint8_t cmd, uint8_t arg) {
    ps2_wait_input();
    outportb(PS2_COMMAND, cmd);
    ps2_wait_input();
    outportb(PS2_DATA, arg);
}

static inline uint8_t ps2_read_byte(void) {
    ps2_wait_output();
    return inportb(PS2_DATA);
}

static inline uint8_t kbd_write(uint8_t byte) {
    ps2_wait_input();
    outportb(PS2_DATA, byte);
    ps2_wait_output();
    return inportb(PS2_DATA);
}

/* write all scancodes directly to the kbd pipe */
static void kbd_handler(__unused struct int_regs *r) {
    uint8_t byte = inportb(PS2_DATA);
    fs_write(kbd_pipe, 0, 1, &byte);
}

void ps2hid_install(void) {
    kbd_pipe = pipe_create(128);
    kbd_pipe->flags = FS_TYPE_CHAR;
    vfs_mount("/dev/kbd", kbd_pipe);

    /* disable all ports when configuring */
    ps2_cmd(PS2_DISABLE_PORT1);
    ps2_cmd(PS2_DISABLE_PORT2);

    /* clear the input buffer */
    short timeout = 1024;
    while((inportb(PS2_STATUS) & BIT(1)) && timeout-- > 0)
        inportb(PS2_DATA);

    if(timeout == 0) {
        puts("ps2hid: no PS/2 found");
        return;
    }

    /* enable interrupt lines and disable translation */
    ps2_cmd(PS2_READ_CONFIG);
    uint8_t status = ps2_read_byte();
    /* currently not using port 2, but the IRQ is still enabled */
    status |= (PS2_PORT1_IRQ | PS2_PORT2_IRQ);
    status &= ~(PS2_PORT1_TLATE);
    ps2_cmd_arg(PS2_WRITE_CONFIG, status);

    /* re-enable ports */
    ps2_cmd(PS2_ENABLE_PORT1);
    ps2_cmd(PS2_ENABLE_PORT2);

    /* enable scan code set 3 */
    /* TODO: fall back to set 2 if set 3 does not exist */
    kbd_write(KBD_SET_SCANCODE);
    kbd_write(3);

    /* install IRQ handlers */
    irq_handler_install(KEYBOARD_IRQ, kbd_handler);
}
