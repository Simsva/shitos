#include "irq.h"
#include <stdint.h>
#include <sys/utils.h>
#include <stdio.h>
#include <kernel/tty/tm.h>
#include "keyboard.h"
#include <kernel/input/usb_keycodes.h>

uint8_t sc_to_usb[128] =
{
    0,  KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E,
    KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACE,
    KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G,
    KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE,
    KEY_LEFTSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N,
    KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, '*', KEY_LEFTALT,
    KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7,
    KEY_KP8, KEY_KP8, KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, KEY_KP6,
    KEY_KPPLUS, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP0, KEY_KPDOT,   0,   0, 0,
    KEY_F11, KEY_F11,
};

static uint8_t mod = 0;

char kb_buf[256] = {0}; //, *head = kb_buf;
static int rear = -1;
static int front = -1;

void enqueue_kb_buf(char c) {
    if (front == -1) {
        front = 0;
    }
    rear += 1;
    kb_buf[rear] = c;
}

void dequeue_kb_buf() {
    front += 1;
}

void flush() {
   for (int i = front; i <= rear; i++) {
        printf("%02x ", kb_buf[i]);
        dequeue_kb_buf();
   }
}

void keyboard_handler() {
    /* Read from keyboard buffer */
    uint16_t scancode = (uint16_t) inb(0x60);
    
    char input_value = sc_to_usb[scancode]; 
  
    /* Key was just released */
    if (scancode & 0x80) {
        
    }
    /* Key was just pressed */
    else {
        enqueue_kb_buf(input_value);
    }
    
    flush();
}

void keyboard_init() {
    irq_handler_install(1, keyboard_handler);
}
