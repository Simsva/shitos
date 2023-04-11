#include <errno.h>
#include <stddef.h>

/* we don't want to bloat the kernel */
#ifndef __is_libk

static const struct errmsgstr {
#define E(n, s) char str##n[sizeof(s)];
#include "__strerror.h"
#undef E
} errmsgstr = {
#define E(n, s) s,
#include "__strerror.h"
#undef E
};

static const unsigned short errmsgidx[] = {
#define E(n, s) [n] = offsetof(struct errmsgstr, str##n),
#include "__strerror.h"
#undef E
};

char *strerror(int e) {
    if((size_t)e >= sizeof errmsgidx / sizeof *errmsgidx) e = 0;
    return (char *)&errmsgstr + errmsgidx[e];
}

#endif /* __is_libk */
