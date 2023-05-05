#include <kernel/fs.h>
#include <kernel/pipe.h>
#include <kernel/arch/i386/ports.h>
#include <kernel/arch/i386/irq.h>
#include <kernel/input/keyboard.h>
#include <kernel/input/usb_keycodes.h>

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

uint8_t sc3_to_usb[] = {
    [0x08] = KEY_ESC,
    [0x16] = KEY_1,
    [0x1E] = KEY_2,
    [0x26] = KEY_3,
    [0x25] = KEY_4,
    [0x2E] = KEY_5,
    [0x36] = KEY_6,
    [0x3D] = KEY_7,
    [0x3E] = KEY_8,
    [0x46] = KEY_9,
    [0x45] = KEY_0,
    [0x4E] = KEY_MINUS,
    [0x55] = KEY_EQUAL,
    [0x66] = KEY_BACKSPACE,
    [0x0D] = KEY_TAB,
    [0x15] = KEY_Q,
    [0x1D] = KEY_W,
    [0x24] = KEY_E,
    [0x2D] = KEY_R,
    [0x2C] = KEY_T,
    [0x35] = KEY_Y,
    [0x3C] = KEY_U,
    [0x43] = KEY_I,
    [0x44] = KEY_O,
    [0x4D] = KEY_P,
    [0x54] = KEY_LEFTBRACE,
    [0x5B] = KEY_RIGHTBRACE,
    [0x5A] = KEY_ENTER,
    [0x11] = KEY_LEFTCTRL,
    [0x1C] = KEY_A,
    [0x1B] = KEY_S,
    [0x23] = KEY_D,
    [0x2B] = KEY_F,
    [0x34] = KEY_G,
    [0x33] = KEY_H,
    [0x3B] = KEY_J,
    [0x42] = KEY_K,
    [0x4B] = KEY_L,
    [0x4C] = KEY_SEMICOLON,
    [0x52] = KEY_APOSTROPHE,
    [0x0E] = KEY_GRAVE,
    [0x12] = KEY_LEFTSHIFT,
    [0x5C] = KEY_BACKSLASH,
    [0x1A] = KEY_Z,
    [0x22] = KEY_X,
    [0x21] = KEY_C,
    [0x2A] = KEY_V,
    [0x32] = KEY_B,
    [0x31] = KEY_N,
    [0x3A] = KEY_M,
    [0x41] = KEY_COMMA,
    [0x49] = KEY_DOT,
    [0x4A] = KEY_SLASH,
    [0x59] = KEY_RIGHTSHIFT,
    //[] = '*',
    [0x19] = KEY_LEFTALT,
    [0x29] = KEY_SPACE,
    [0x14] = KEY_CAPSLOCK,
    [0x07] = KEY_F1,
    [0x0F] = KEY_F2,
    [0x17] = KEY_F3,
    [0x1F] = KEY_F4,
    [0x27] = KEY_F5,
    [0x2F] = KEY_F6,
    [0x37] = KEY_F7,
    [0x3F] = KEY_F8,
    [0x47] = KEY_F9,
    [0x4F] = KEY_F10,
    [0x76] = KEY_NUMLOCK,
    [0x5F] = KEY_SCROLLLOCK,
    [0x6C] = KEY_KP7,
    [0x75] = KEY_KP8,
    [0x7D] = KEY_KP9,
    /* [0x4E] = KEY_KPMINUS, */
    [0x6B] = KEY_KP4,
    [0x73] = KEY_KP5,
    [0x74] = KEY_KP6,
    [0x7C] = KEY_KPPLUS,
    [0x69] = KEY_KP1,
    [0x72] = KEY_KP2,
    [0x7A] = KEY_KP3,
    [0x70] = KEY_KP0,
    [0x71] = KEY_KPDOT,
    [0x56] = KEY_F11,
    [0x5E] = KEY_F12,
    [0x6A] = KEY_RIGHT,
    [0x61] = KEY_LEFT,
    [0x60] = KEY_DOWN,
    [0x63] = KEY_UP,
};

static uint8_t mod = 0;
static uint8_t not_released = 1;
static uint8_t send = 1;

void change_mod(int modkey) {
    if (not_released) {
        mod = modkey; 
    }
    else {
        mod = 0;
    }
    send = 0;
}

fs_node_t *kbd_pipe = NULL;

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
    /* Read from keyboard buffer */
    uint8_t scancode = inportb(PS2_DATA);
    uint8_t keycode = sc3_to_usb[scancode];
    
    switch(keycode) {
        case KEY_LEFTSHIFT:
            change_mod(KEY_MOD_LSHIFT); 
            break;
        case KEY_LEFTCTRL:
            change_mod(KEY_MOD_LCTRL);
            break;
    }

    if (scancode == 0xF0) {
        //printf("test");
        not_released = 0;
    }
    else {
        if (send) { 
            struct kb_packet key_press = {
                .keycode = keycode,
                .mod = mod,
                .release_flag = not_released, 
            };

            handle_kb_input(key_press);    
        }
        else {
            send = 1;
        }
        not_released = 1;
    }

}

void ps2hid_install(void) {
    kbd_pipe = pipe_create(128);
    kbd_pipe->flags = FS_FLAG_IFCHR;
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


