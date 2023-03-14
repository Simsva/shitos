#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init();
void enqueue_kb_buf(char c);
void dequeue_kb_buf();
void flush();

#endif
