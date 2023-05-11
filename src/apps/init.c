#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <string.h>

#define MAX_STR 32
#define MAX_ARGV 16

#define LEN(arr) (sizeof(arr)/sizeof(*(arr)))

int strnsplit(char *restrict s, const char *restrict delim, char **v, int n) {
  char *pch, *save;
  int c = 0;
  pch = strtok_r(s, delim, &save);
  while(n-- && pch != NULL) v[c++] = pch, pch = strtok_r(NULL, delim, &save);
  return c;
}

void ls(int argc, char **argv) {
    
}

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    /* emulate blocking I/O in libc */
    stdin->flags |= 4;

    /* TODO: ascii + display characters as they are typed */
    /* this is all disgusting jank because we don't have proper ttys */
    char buf[MAX_STR], *argv2[MAX_ARGV];
    int argc2;

    for(;;) {
        printf("$ ");
        fflush(stdout);
        fgets(buf, sizeof buf, stdin);
        // printf("you entered: %s", buf);
        
        argc2 = strnsplit(buf, " ", argv2, LEN(argv2));

        if (!strcmp(argv2[0], "ls") {
            ls(argc2, argv2);
        }

        /* for (int i = 0; i < argc2; i++) { */
        /*   printf("arg %d: %s\n", i, argv2[i]); */
        /* } */
    }

    return EXIT_SUCCESS;
}
