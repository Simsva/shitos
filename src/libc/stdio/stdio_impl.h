#ifndef STDIO_IMPL_H_
#define STDIO_IMPL_H_

#include <stdio.h>

#define F_EOF   0x01
#define F_ERR   0x02
#define F_BLOCK 0x04

extern __hidden FILE *__head;

extern __hidden FILE __stdin_FILE;
extern __hidden FILE __stdout_FILE;
extern __hidden FILE __stderr_FILE;

int __stdio_close(FILE *);
size_t __stdio_read(FILE *, unsigned char *, size_t);
size_t __stdio_write(FILE *, const unsigned char *, size_t);
int __stdio_seek(FILE *, off_t, int);

unsigned __fmodeflags(const char *);

#endif // STDIO_IMPL_H_
