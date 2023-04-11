#include <kernel/input/keyboard.h>
#include <stdio.h>
#include <kernel/input/usb_keycodes.h>

static const char *us_keymap[][256] = {
    { /* bare */
        [KEY_A] = "a",
        [KEY_B] = "b",
        [KEY_C] = "c",
        [KEY_D] = "d",
        [KEY_E] = "e",
        [KEY_F] = "f",
        [KEY_G] = "g",
        [KEY_H] = "h",
        [KEY_I] = "i",
        [KEY_J] = "j",
        [KEY_K] = "k",
        [KEY_L] = "l",
        [KEY_M] = "m",
        [KEY_N] = "n",
        [KEY_O] = "o",
        [KEY_P] = "p",
        [KEY_Q] = "q",
        [KEY_R] = "r",
        [KEY_S] = "s",
        [KEY_T] = "t",
        [KEY_U] = "u",
        [KEY_V] = "v",
        [KEY_W] = "w",
        [KEY_X] = "x",
        [KEY_Y] = "y",
        [KEY_Z] = "z",
        [KEY_UP] = "\033[A",
        [KEY_DOWN] = "\033[B",
        [KEY_RIGHT] = "\033[C",
        [KEY_LEFT] = "\033[D",
    },
    { /* shift */

    },
    { /* third level shift (AltGr) */
    },
    { /* ctrl */
    }
};

void handle_kb_input(struct kb_packet keypress) {
    /* 
    TODO:
       Convert key code to ascii
       Check release flag
       Check mod

    */
    
    printf("%x ", us_keymap[0][keypress.keycode]);
}

