#include <kernel/input/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <kernel/input/usb_keycodes.h>
#include <kernel/fs.h>

extern fs_node_t *kbd_pipe;

static const char *us_keymap[][256] = {
    { /* bare */
        [KEY_ENTER] = "\n",
        [KEY_0] = "0",
        [KEY_1] = "1",
        [KEY_2] = "2",
        [KEY_3] = "3",
        [KEY_4] = "4",
        [KEY_5] = "5",
        [KEY_6] = "6",
        [KEY_7] = "7",
        [KEY_8] = "8",
        [KEY_9] = "9",
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
        [KEY_SPACE] = " ",
    },
    { /* ctrl */
       [KEY_0] = "\0",
       [KEY_1] = "\0",
       [KEY_2] = "\0",
       [KEY_3] = "\0",
       [KEY_4] = "\0",
       [KEY_5] = "\0",
       [KEY_6] = "\0",
       [KEY_7] = "\0",
       [KEY_8] = "\0",
       [KEY_9] = "\0",
       [KEY_A] = "\001",
       [KEY_B] = "\002",
       [KEY_C] = "\003",
       [KEY_D] = "\004",
       [KEY_E] = "\005",
       [KEY_F] = "\006",
       [KEY_G] = "\007",
       [KEY_H] = "\010",
       [KEY_I] = "\011",
       [KEY_J] = "\012",
       [KEY_K] = "\013",
       [KEY_L] = "\014",
       [KEY_M] = "\015",
       [KEY_N] = "\016",
       [KEY_O] = "\017",
       [KEY_P] = "\020",
       [KEY_Q] = "\021",
       [KEY_R] = "\022",
       [KEY_S] = "\023",
       [KEY_T] = "\024",
       [KEY_U] = "\025",
       [KEY_V] = "\026",
       [KEY_W] = "\027",
       [KEY_X] = "\030",
       [KEY_Y] = "\031",
       [KEY_Z] = "\032",
    },
    { /* left shift */
        [KEY_0] = ")",
        [KEY_1] = "!",
        [KEY_2] = "@",
        [KEY_3] = "#",
        [KEY_4] = "$",
        [KEY_5] = "%",
        [KEY_6] = "^",
        [KEY_7] = "&",
        [KEY_8] = "*",
        [KEY_9] = "(",
        [KEY_A] = "A",
        [KEY_B] = "B",
        [KEY_C] = "C",
        [KEY_D] = "D",
        [KEY_E] = "E",
        [KEY_F] = "F",
        [KEY_G] = "G",
        [KEY_H] = "H",
        [KEY_I] = "I",
        [KEY_J] = "J",
        [KEY_K] = "K",
        [KEY_L] = "L",
        [KEY_M] = "M",
        [KEY_N] = "N",
        [KEY_O] = "O",
        [KEY_P] = "P",
        [KEY_Q] = "Q",
        [KEY_R] = "R",
        [KEY_S] = "S",
        [KEY_T] = "T",
        [KEY_U] = "U",
        [KEY_V] = "V",
        [KEY_W] = "W",
        [KEY_X] = "X",
        [KEY_Y] = "Y",
        [KEY_Z] = "Z",


    },
    { /* Alt Gr */
    }
};

void handle_kb_input(struct kb_packet keypress) {
    
    /* todo:
        * key to ascii 
    */
    
    if (keypress.release_flag) {
        const char *str = us_keymap[keypress.mod][keypress.keycode];
        if(str) {
            printf("%s", str);
            fs_write(kbd_pipe, 0, strlen(str), str);
        }

    }

    //fs_write(kbd_pipe, 0, 1, "c");
}

