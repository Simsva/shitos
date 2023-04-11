#ifndef KERNEL_INPUT_KEYBOARD_H_
#define KERNEL_INPUT_KEYBOARD_H_

#include <stdint.h>


struct kb_packet {
    uint8_t keycode;
    uint8_t mod;
    uint8_t release_flag;
};

void handle_kb_input(struct kb_packet key_press);

#endif // KERNEL_INPUT_KEYBOARD_H_
