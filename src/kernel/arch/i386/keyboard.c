#include "irq.h"
#include <stdint.h>
#include <sys/utils.h>
#include <stdio.h>
#include <kernel/tty/tm.h>
#include "keyboard.h"

unsigned char kbl_us[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* backspace */
  '\t',			/* tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* enter key */
    0,			/* 29   - control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* right shift */
  '*',
    0,	/* alt */
  ' ',	/* space bar */
    0,	/* caps lock */
    0,	/* 59 - f1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... f10 */
    0,	/* 69 - num lock*/
    0,	/* scroll lock */
    0,	/* home key */
    0,	/* up arrow */
    0,	/* page up */
  '-',
    0,	/* left arrow */
    0,
    0,	/* right arrow */
  '+',
    0,	/* 79 - end key*/
    0,	/* down arrow */
    0,	/* page down */
    0,	/* insert key */
    0,	/* delete key */
    0,   0,   0,
    0,	/* f11 key */
    0,	/* f12 key */
    0,	/* all other keys are undefined */
};

char kb_buf[256] = {0}; //, *head = kb_buf;
int rear = -1;
int front = -1;

void flush() {
   for (int i = front; i <= rear; i++) {
        char c = kb_buf[i];
        putchar(c);
        front += 1;
   }
}

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

void keyboard_handler() {
    uint16_t scancode = (uint16_t) inb(0x60);
    
    char input_value = kbl_us[scancode]; 
  
    /* Key was just released */
    if (scancode & 0x80) {
        
    }
    /* Key was just pressed */
    else {
        enqueue_kb_buf(input_value);
        //*head++ = input_value;
    }
    
    flush();
}

void keyboard_init() {
    irq_handler_install(1, keyboard_handler);
}
