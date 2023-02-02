#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define F_ALT_FORM (UINT32_C(1)<<('#'-' '))
#define F_ZERO_PAD (UINT32_C(1)<<('0'-' '))
#define F_LEFT_ADJ (UINT32_C(1)<<('-'-' '))
#define F_PAD_POS  (UINT32_C(1)<<(' '-' '))
#define F_MARK_POS (UINT32_C(1)<<('+'-' '))
#define F_GROUPED  (UINT32_C(1)<<('\''-' '))

#define FLAGMASK (F_ALT_FORM|F_ZERO_PAD|F_LEFT_ADJ|F_PAD_POS|F_MARK_POS|F_GROUPED)

enum {
    BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE, ZTPRE, JPRE,
    STOP,
    CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LLONG, ULLONG,
    IMAX, UMAX, PTR, PDIFF, UIPTR, SIZET,
    NOARG,
    MAXSTATE,
};

#define S(c) [(c) - 'A']

static const unsigned char states[]['z' - 'A' + 1] = {
    { /* 0: bare types */
        S('d') = INT, S('i') = INT,
        S('u') = UINT, S('o') = UINT, S('x') = UINT, S('X') = UINT,
        S('c') = CHAR, S('s') = PTR,
        S('p') = UIPTR, S('n') = PTR,

        S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE,
        S('z') = ZTPRE, S('t') = ZTPRE, S('j') = JPRE,
    }, { /* 1: l-prefixed */
        S('d') = LONG, S('i') = LONG,
        S('u') = ULONG, S('o') = ULONG, S('x') = ULONG, S('X') = ULONG,
        S('n') = PTR,
        /* NOTE: not implemented */
        /* S('c') = INT, S('s') = PTR, */

        S('l') = LLPRE,
    }, { /* 2: ll-prefixed */
        S('d') = LLONG, S('i') = LLONG,
        S('u') = ULLONG, S('o') = ULLONG, S('x') = ULLONG, S('X') = ULLONG,
        S('n') = PTR,
    }, { /* 3: h-prefixed */
        S('d') = SHORT, S('i') = SHORT,
        S('u') = USHORT, S('o') = USHORT, S('x') = USHORT, S('X') = USHORT,
        S('n') = PTR,

        S('h') = HHPRE,
    }, { /* 4: hh-prefixed */
        S('d') = CHAR, S('i') = CHAR,
        S('u') = UCHAR, S('o') = UCHAR, S('x') = UCHAR, S('X') = UCHAR,
        S('n') = PTR,
    }, { /* 5: L-prefixed */
        /* for floating point args, unimplemented */
    }, { /* 6: z- or t-prefixed (assumed to be same size) */
        S('d') = PDIFF, S('i') = PDIFF,
        S('u') = SIZET, S('o') = SIZET, S('x') = SIZET, S('X') = SIZET,
        S('n') = PTR,
    }, { /* 7: j-prefixed */
        S('d') = IMAX, S('i') = IMAX,
        S('u') = UMAX, S('o') = UMAX, S('x') = UMAX, S('X') = UMAX,
        S('n') = PTR,
    }
};

#define OOB(x) ((unsigned)(x) - 'A' > 'z' - 'A')

union arg {
    uintmax_t i;
    void *p;
};

static void pop_arg(union arg *arg, int type, va_list *ap) {
    switch(type) {
    case PTR:    arg->p = va_arg(*ap, void *); break;
    case CHAR:   arg->i = (signed char)va_arg(*ap, int); break;
    case UCHAR:  arg->i = (unsigned char)va_arg(*ap, int); break;
    case SHORT:  arg->i = (short)va_arg(*ap, int); break;
    case USHORT: arg->i = (unsigned short)va_arg(*ap, int); break;
    case INT:    arg->i = va_arg(*ap, int); break;
    case UINT:   arg->i = va_arg(*ap, unsigned int); break;
    case LONG:   arg->i = va_arg(*ap, long); break;
    case ULONG:  arg->i = va_arg(*ap, unsigned long); break;
    case LLONG:  arg->i = va_arg(*ap, long long); break;
    case ULLONG: arg->i = va_arg(*ap, unsigned long long); break;
    case IMAX:   arg->i = va_arg(*ap, intmax_t); break;
    case UMAX:   arg->i = va_arg(*ap, uintmax_t); break;
    case PDIFF:  arg->i = va_arg(*ap, ptrdiff_t); break;
    case UIPTR:  arg->i = (uintptr_t)va_arg(*ap, void *); break;
    case SIZET:  arg->i = va_arg(*ap, size_t); break;
    }
}

static int getint(char **s) {
    int i;
    for(i = 0; isdigit(**s); (*s)++) {
        if((unsigned)i > INT_MAX/10U || **s - '0' > INT_MAX-10*i) i = -1;
        else i = 10*i + (**s - '0');
    }
    return i;
}

static void output(const char *s, int l) {
    while(l-- > 0) putchar(*s++);
}

static void pad(char c, int w, int l, int fl) {
    char pad[256];
    if(fl & (F_LEFT_ADJ | F_ZERO_PAD) || l >= w) return;
    l = w - l;
    memset(pad, c, (size_t)l > sizeof pad ? sizeof pad : (size_t)l);
    for(; (unsigned)l >= sizeof pad; l -= sizeof pad)
        output(pad, sizeof(pad));
    output(pad, l);
}

static const char xdigits[16] = {
    "0123456789ABCDEF"
};

static char *fmt_x(uintmax_t x, char *s, int lower) {
    for(; x; x>>=4) *--s = xdigits[x & 15] | lower;
    return s;
}

static char *fmt_o(uintmax_t x, char *s) {
    for(; x; x>>=3) *--s = '0' + (x & 7);
    return s;
}

static char *fmt_u(uintmax_t x, char *s) {
    unsigned long y;
    for(     ; x > ULONG_MAX; x /= 10) *--s = '0' + x%10;
    for(y = x;             y; y /= 10) *--s = '0' + y%10;
    return s;
}

static int printf_core(const char *fmt, va_list *ap, union arg *nl_arg, int *nl_type) {
    char *a, *z, *s = (char *)fmt;
    unsigned l10n = 0;
    uint32_t fl;
    int w, p, xp;
    union arg arg;
    int argi;
    unsigned st, ps;
    int cnt = 0, l = 0;
    char buf[sizeof(uintmax_t)*3];
    const char *prefix;
    int t, pl;

    for(;;) {
        if(l > INT_MAX - cnt) goto overflow;

        /* update output count, end loop when fmt is exhausted */
        cnt += l;
        if(!*s) break;

        /* literal text and %% format specifiers */
        for(a = s; *s && *s != '%'; s++);
        for(z = s; s[0]=='%' && s[1] == '%'; z++, s+=2);
        if(z-a > INT_MAX-cnt) goto overflow;
        l = z-a;
        output(a, l);
        if(l) continue;

        if(isdigit(s[1]) && s[2] == '$') {
            l10n = 1;
            argi = s[1] - '0';
            s += 3;
        } else {
            argi = -1;
            s++;
        }

        /* read flags */
        for(fl = 0; (uint32_t)*s-' ' < 32 && (FLAGMASK & (UINT32_C(1)<<(*s - ' '))); s++)
            fl |= UINT32_C(1)<<(*s - ' ');

        /* read field width */
        if(*s == '*') {
            if(isdigit(s[1]) && s[2] == '$') {
                l10n = 1;
                nl_type[s[1] - '0'] = INT;
                w = nl_arg[s[1] - '0'].i;
                s += 3;
            } else if(!l10n) {
                w = va_arg(*ap, int);
                s++;
            } else goto inval;
            if(w < 0) fl |= F_LEFT_ADJ, w = -w;
        } else if((w = getint(&s)) < 0) goto overflow;

        /* read precision */
        if(*s == '.' && s[1] == '*') {
            if(isdigit(s[2]) && s[3] == '$') {
                nl_type[s[2] - '0'] = INT;
                p = nl_arg[s[2] - '0'].i;
                s += 4;
            } else if(!l10n) {
                p = va_arg(*ap, int);
                s += 2;
            } else goto inval;
            xp = (p >= 0);
        } else if(*s == '.') {
            s++;
            p = getint(&s);
            xp = 1;
        } else {
            p = -1;
            xp = 0;
        }

        /* format specifier state machine */
        st = 0;
        do {
            if(OOB(*s)) goto inval;
            ps = st;
            st = states[st]S(*s++);
        } while(st - 1 < STOP);
        if(!st) goto inval;

        /* check validity of argument type */
        if(st == NOARG) {
            if(argi >= 0) goto inval;
        } else {
            if(argi >= 0) nl_type[argi] = st, arg = nl_arg[argi];
            else pop_arg(&arg, st, ap);
        }

        z = buf + sizeof(buf);
        prefix = "-+   0X0x";
        pl = 0;
        t = s[-1];

        /* NOTE: not implemented */
        /* transform ls/lc to S/C */
        /* if(ps && (t & 0xf) == 3) t &= ~0x20; */

        /* - and 0 flags are mutually exclusive */
        if(fl & F_LEFT_ADJ) fl &= ~F_ZERO_PAD;

        switch(t) {
        case 'n':
            switch(ps) {
            case BARE:  *(int *)arg.p = cnt; break;
            case LPRE:  *(long *)arg.p = cnt; break;
            case LLPRE: *(long long *)arg.p = cnt; break;
            case HPRE:  *(unsigned short *)arg.p = cnt; break;
            case HHPRE: *(unsigned char *)arg.p = cnt; break;
            case ZTPRE: *(size_t *)arg.p = cnt; break;
            case JPRE:  *(uintmax_t *)arg.p = cnt; break;
            }
            continue;
        case 'p':
            p = MAX((unsigned)p, 2*sizeof(void *));
            t = 'x';
            fl |= F_ALT_FORM;
            __attribute__((fallthrough));
        case 'x': case 'X':
            a = fmt_x(arg.i, z, t&32);
            if(arg.i && (fl & F_ALT_FORM)) prefix += (t>>4), pl=2;
            if(0) {
        case 'o':
            a = fmt_o(arg.i, z);
            if((fl & F_ALT_FORM) && p < z-a+1) p = z-a+1;
            } if(0) {
        case 'd': case 'i':
            pl = 1;
            if(arg.i > INTMAX_MAX) {
                arg.i = -arg.i;
            } else if(fl & F_MARK_POS) {
                prefix++;
            } else if(fl & F_PAD_POS) {
                prefix += 2;
            } else pl = 0;
        case 'u':
            a = fmt_u(arg.i, z);
            }
            if(xp && p < 0) goto overflow;
            if(xp) fl &= ~F_ZERO_PAD;
            if(!arg.i && !p) {
                a = z;
                break;
            }
            p = MAX(p, z-a + !arg.i);
            break;

        case 'c':
            *(a = z - (p = 1)) = arg.i;
            fl &= ~F_ZERO_PAD;
            break;

        /* TODO: case 'm': */
        case 's':
            a = arg.p ? arg.p : "(null)";
            z = a + strnlen(a, p<0 ? INT_MAX : p);
            if(p<0 && *z) goto overflow;
            p = z - a;
            fl &= ~F_ZERO_PAD;
            break;

        /* float */
        }

        if(p < z-a) p = z-a;
        if(p > INT_MAX-pl) goto overflow;
        if(w < pl+p) w = pl+p;
        if(w > INT_MAX-cnt) goto overflow;

        pad(' ', w, pl+p, fl);
        output(prefix, pl);
        pad('0', w, pl+p, fl^F_ZERO_PAD);
        pad('0', p, z-a, 0);
        output(a, z-a);
        pad(' ', w, pl+p, fl^F_LEFT_ADJ);

        l = w;
    }

    return cnt;

    /* TODO: errno */
inval:
    return EOF;
overflow:
    return EOF;
}

int vprintf(const char *restrict fmt, va_list ap) {;
    int nl_type[NL_ARGMAX+1] = { 0 };
    union arg nl_arg[NL_ARGMAX+1] = { 0 };
    int r;

    r = printf_core(fmt, &ap, nl_arg, nl_type);

    return r;
}
