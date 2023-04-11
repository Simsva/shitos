#ifndef DIRENT_H_
#define DIRENT_H_

#include <_cheader.h>
#include <sys/types.h>

_BEGIN_C_HEADER

struct dirent {
    ino_t d_ino;
    char d_name[256];
};

typedef struct DIR {
    int fd;
    int cur_entry;
} DIR;

DIR *opendir(const char *dirname);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);

_END_C_HEADER

#endif // DIRENT_H_
