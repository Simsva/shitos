#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define LEN(arr) (sizeof(arr) / sizeof(*(arr)))

#define MAX_STR 512
#define MAX_ARGV 16
#define MAX_PATH PATH_MAX /* 4096 */

int strnsplit(char *restrict s, const char *restrict delim, char **v, int n) {
    char *pch, *save;
    int c = 0;
    pch = strtok_r(s, delim, &save);
    while(n-- && pch != NULL)
        v[c++] = pch, pch = strtok_r(NULL, delim, &save);
    return c;
}

void pwd(char new_line) {
    char cwd[MAX_PATH];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s", cwd);
        putchar(new_line);
    }
    else
        perror("error");
}


void ls(int argc, char **argv) {
    char *path;

    path = argc > 1 ? strdup(argv[1]) : getcwd(NULL, 0);  

    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    free(path);
    if(d != NULL) {
        errno = 0;
        while((dir = readdir(d)) != NULL) {
            printf("%s ", dir->d_name);
        }
        if(errno) perror("readdir");
        else putchar('\n');
        closedir(d);
    } else
        perror("opendir");
}


void cd(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "/";
    if (chdir(path) != 0)
        perror("error");
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
        pwd('\0');
        printf(" $ ");
        fflush(stdout);

        fgets(buf, sizeof buf, stdin);
        buf[strcspn(buf, "\n")] = '\0';
        argc2 = strnsplit(buf, " ", argv2, LEN(argv2)); 

        if(!strcmp(argv2[0], "pwd"))
            pwd('\n');
        else if(!strcmp(argv2[0], "ls"))
            ls(argc2, argv2);
        else if(!strcmp(argv2[0], "cd"))
            cd(argc2, argv2);
        else
            puts("No such command!");

    }

    return EXIT_SUCCESS;
}
