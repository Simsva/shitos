#include "stdio_impl.h"
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>
#include <errno.h>

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
    DBL, LDBL,
    NOARG,
    MAXSTATE,
};

#define S(c) [(c) - 'A']

/*
** Unimplemented:
** 0: C, S
** 1: c, s
*/
static const unsigned char states[]['z' - 'A' + 1] = {
    { /* 0: bare types */
        S('d') = INT, S('i') = INT,
        S('u') = UINT, S('o') = UINT, S('x') = UINT, S('X') = UINT,
        S('c') = CHAR, S('s') = PTR,
        S('p') = UIPTR, S('n') = PTR,

        S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
        S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,

        S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE,
        S('z') = ZTPRE, S('t') = ZTPRE, S('j') = JPRE,
    }, { /* 1: l-prefixed */
        S('d') = LONG, S('i') = LONG,
        S('u') = ULONG, S('o') = ULONG, S('x') = ULONG, S('X') = ULONG,
        S('n') = PTR,

        S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
        S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,

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
        S('e') = LDBL, S('f') = LDBL, S('g') = LDBL, S('a') = LDBL,
        S('E') = LDBL, S('F') = LDBL, S('G') = LDBL, S('A') = LDBL,
        S('n') = PTR,
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
    long double f;
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
    case DBL:    arg->f = va_arg(*ap, double); break;
    case LDBL:   arg->f = va_arg(*ap, long double); break;
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

static void output(FILE *f, const char *s, int l) {
    if(!f || !f->write) return;
    f->write(f, (uint8_t *)s, l);
}

static void pad(FILE *f, char c, int w, int l, int fl) {
    if(!f) return;
    char pad[256];
    if(fl & (F_LEFT_ADJ | F_ZERO_PAD) || l >= w) return;
    l = w - l;
    memset(pad, c, (size_t)l > sizeof pad ? sizeof pad : (size_t)l);
    for(; (unsigned)l >= sizeof pad; l -= sizeof pad)
        output(f, pad, sizeof(pad));
    output(f, pad, l);
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

static int fmt_fp(FILE *f, long double y, int w, int p, int fl, int t) {
    uint32_t big[(LDBL_MANT_DIG+28)/29 + 1      /* mantissa expansion */
        + (LDBL_MAX_EXP+LDBL_MANT_DIG+28+8)/9]; /* exponent expansion */
    uint32_t *a, *d, *r, *z;
    int e2=0, e, i, j, l;
    char buf[9+LDBL_MANT_DIG/4], *s;
    const char *prefix="-0X+0X 0X-0x+0x 0x";
    int pl;
    char ebuf0[3*sizeof(int)], *ebuf=&ebuf0[3*sizeof(int)], *estr = NULL;

    pl=1;
    if (signbit(y)) {
        y=-y;
    } else if (fl & F_MARK_POS) {
        prefix+=3;
    } else if (fl & F_PAD_POS) {
        prefix+=6;
    } else prefix++, pl=0;

    if (!isfinite(y)) {
        char *s = (t&32)?"inf":"INF";
        if (y!=y) s=(t&32)?"nan":"NAN";
        pad(f, ' ', w, 3+pl, fl&~F_ZERO_PAD);
        output(f, prefix, pl);
        output(f, s, 3);
        pad(f, ' ', w, 3+pl, fl^F_LEFT_ADJ);
        return MAX(w, 3+pl);
    }

    y = frexpl(y, &e2) * 2;
    if (y) e2--;

    if ((t|32)=='a') {
        long double round = 8.0;
        int re;

        if (t&32) prefix += 9;
        pl += 2;

        if (p<0 || p>=LDBL_MANT_DIG/4-1) re=0;
        else re=LDBL_MANT_DIG/4-1-p;

        if (re) {
            round *= 1<<(LDBL_MANT_DIG%4);
            while (re--) round*=16;
            if (*prefix=='-') {
                y=-y;
                y-=round;
                y+=round;
                y=-y;
            } else {
                y+=round;
                y-=round;
            }
        }

        estr=fmt_u(e2<0 ? -e2 : e2, ebuf);
        if (estr==ebuf) *--estr='0';
        *--estr = (e2<0 ? '-' : '+');
        *--estr = t+('p'-'a');

        s=buf;
        do {
            int x=y;
            *s++=xdigits[x]|(t&32);
            y=16*(y-x);
            if (s-buf==1 && (y||p>0||(fl&F_ALT_FORM))) *s++='.';
        } while (y);

        if (p > INT_MAX-2-(ebuf-estr)-pl)
            return -1;
        if (p && s-buf-2 < p)
            l = (p+2) + (ebuf-estr);
        else
            l = (s-buf) + (ebuf-estr);

        pad(f, ' ', w, pl+l, fl);
        output(f, prefix, pl);
        pad(f, '0', w, pl+l, fl^F_ZERO_PAD);
        output(f, buf, s-buf);
        pad(f, '0', l-(ebuf-estr)-(s-buf), 0, 0);
        output(f, estr, ebuf-estr);
        pad(f, ' ', w, pl+l, fl^F_LEFT_ADJ);
        return MAX(w, pl+l);
    }
    if (p<0) p=6;

    if (y) y *= 0x1p28, e2-=28;

    if (e2<0) a=r=z=big;
    else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

    do {
        *z = y;
        y = 1000000000*(y-*z++);
    } while (y);

    while (e2>0) {
        uint32_t carry=0;
        int sh=MIN(29,e2);
        for (d=z-1; d>=a; d--) {
            uint64_t x = ((uint64_t)*d<<sh)+carry;
            *d = x % 1000000000;
            carry = x / 1000000000;
        }
        if (carry) *--a = carry;
        while (z>a && !z[-1]) z--;
        e2-=sh;
    }
    while (e2<0) {
        uint32_t carry=0, *b;
        int sh=MIN(9,-e2), need=1+(p+LDBL_MANT_DIG/3U+8)/9;
        for (d=a; d<z; d++) {
            uint32_t rm = *d & ((1<<sh)-1);
            *d = (*d>>sh) + carry;
            carry = (1000000000>>sh) * rm;
        }
        if (!*a) a++;
        if (carry) *z++ = carry;
        /* Avoid (slow!) computation past requested precision */
        b = (t|32)=='f' ? r : a;
        if (z-b > need) z = b+need;
        e2+=sh;
    }

    if (a<z) for (i=10, e=9*(r-a); *a>=(uint32_t)i; i*=10, e++);
    else e=0;

    /* Perform rounding: j is precision after the radix (possibly neg) */
    j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
    if (j < 9*(z-r-1)) {
        uint32_t x;
        /* We avoid C's broken division of negative numbers */
        d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
        j += 9*LDBL_MAX_EXP;
        j %= 9;
        for (i=10, j++; j<9; i*=10, j++);
        x = *d % i;
        /* Are there any significant digits past j? */
        if (x || d+1!=z) {
            long double round = 2/LDBL_EPSILON;
            long double small;
            if ((*d/i & 1) || (i==1000000000 && d>a && (d[-1]&1)))
                round += 2;
            if (x<(uint32_t)i/2) small=0x0.8p0;
            else if (x==(uint32_t)i/2 && d+1==z) small=0x1.0p0;
            else small=0x1.8p0;
            if (pl && *prefix=='-') round*=-1, small*=-1;
            *d -= x;
            /* Decide whether to round by probing round+small */
            if (round+small != round) {
                *d = *d + i;
                while (*d > 999999999) {
                    *d--=0;
                    if (d<a) *--a=0;
                    (*d)++;
                }
                for (i=10, e=9*(r-a); *a>=(uint32_t)i; i*=10, e++);
            }
        }
        if (z>d+1) z=d+1;
    }
    for (; z>a && !z[-1]; z--);

    if ((t|32)=='g') {
        if (!p) p++;
        if (p>e && e>=-4) {
            t--;
            p-=e+1;
        } else {
            t-=2;
            p--;
        }
        if (!(fl&F_ALT_FORM)) {
            /* Count trailing zeros in last place */
            if (z>a && z[-1]) for (i=10, j=0; z[-1]%i==0; i*=10, j++);
            else j=9;
            if ((t|32)=='f')
                p = MIN(p,MAX(0,9*(z-r-1)-j));
            else
                p = MIN(p,MAX(0,9*(z-r-1)+e-j));
        }
    }
    if (p > INT_MAX-1-(p || (fl&F_ALT_FORM)))
        return -1;
    l = 1 + p + (p || (fl&F_ALT_FORM));
    if ((t|32)=='f') {
        if (e > INT_MAX-l) return -1;
        if (e>0) l+=e;
    } else {
        estr=fmt_u(e<0 ? -e : e, ebuf);
        while(ebuf-estr<2) *--estr='0';
        *--estr = (e<0 ? '-' : '+');
        *--estr = t;
        if (ebuf-estr > INT_MAX-l) return -1;
        l += ebuf-estr;
    }

    if (l > INT_MAX-pl) return -1;
    pad(f, ' ', w, pl+l, fl);
    output(f, prefix, pl);
    pad(f, '0', w, pl+l, fl^F_ZERO_PAD);

    if ((t|32)=='f') {
        if (a>r) a=r;
        for (d=a; d<=r; d++) {
            char *s = fmt_u(*d, buf+9);
            if (d!=a) while (s>buf) *--s='0';
            else if (s==buf+9) *--s='0';
            output(f, s, buf+9-s);
        }
        if (p || (fl&F_ALT_FORM)) output(f, ".", 1);
        for (; d<z && p>0; d++, p-=9) {
            char *s = fmt_u(*d, buf+9);
            while (s>buf) *--s='0';
            output(f, s, MIN(9,p));
        }
        pad(f, '0', p+9, 9, 0);
    } else {
        if (z<=a) z=a+1;
        for (d=a; d<z && p>=0; d++) {
            char *s = fmt_u(*d, buf+9);
            if (s==buf+9) *--s='0';
            if (d!=a) while (s>buf) *--s='0';
            else {
                output(f, s++, 1);
                if (p>0||(fl&F_ALT_FORM)) output(f, ".", 1);
            }
            output(f, s, MIN(buf+9-s, p));
            p -= buf+9-s;
        }
        pad(f, '0', p+18, 18, 0);
        output(f, estr, ebuf-estr);
    }

    pad(f, ' ', w, pl+l, fl^F_LEFT_ADJ);

    return MAX(w, pl+l);
}

static int printf_core(FILE *f, const char *fmt, va_list *ap, union arg *nl_arg, int *nl_type) {
    char *a, *z, *s = (char *)fmt;
    unsigned l10n = 0;
    uint32_t fl;
    int w, p, xp;
    union arg arg;
    int argi;
    unsigned st, ps;
    int cnt = 0, l = 0;
    size_t i;
    char buf[sizeof(uintmax_t)*3 + 3 + LDBL_MANT_DIG/4];
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
        if(f) output(f, a, l);
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
                w = f ? va_arg(*ap, int) : 0;
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
                p = f ? va_arg(*ap, int) : 0;
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
            else if(f) pop_arg(&arg, st, ap);
            else return 0;
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
            __fallthrough;
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

        case 'e': case 'f': case 'g': case 'a':
        case 'E': case 'F': case 'G': case 'A':
            if (xp && p<0) goto overflow;
            l = fmt_fp(f, arg.f, w, p, fl, t);
            if (l<0) goto overflow;
            continue;
        }

        if(p < z-a) p = z-a;
        if(p > INT_MAX-pl) goto overflow;
        if(w < pl+p) w = pl+p;
        if(w > INT_MAX-cnt) goto overflow;

        pad(f, ' ', w, pl+p, fl);
        output(f, prefix, pl);
        pad(f, '0', w, pl+p, fl^F_ZERO_PAD);
        pad(f, '0', p, z-a, 0);
        output(f, a, z-a);
        pad(f, ' ', w, pl+p, fl^F_LEFT_ADJ);

        l = w;
    }

    if(f) return cnt;
    if(!l10n) return 0;

    /* read nl args */
    for(i = 1; i <= NL_ARGMAX && nl_type[i]; i++)
        pop_arg(nl_arg+i, nl_type[i], ap);
    for(; i <= NL_ARGMAX && !nl_type[i]; i++);
    if(i <= NL_ARGMAX) goto inval;
    return 1;

inval:
    errno = EINVAL;
    return EOF;
overflow:
    errno = EOVERFLOW;
    return EOF;
}

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap) {
    va_list ap2;
    int nl_type[NL_ARGMAX+1] = { 0 };
    union arg nl_arg[NL_ARGMAX+1] = { 0 };
    int r;

    /* musl libc told me this was a good idea */
    va_copy(ap2, ap);
    if(printf_core(NULL, fmt, &ap2, nl_arg, nl_type) < 0) {
        va_end(ap2);
        return -1;
    }

    r = printf_core(f, fmt, &ap2, nl_arg, nl_type);

    va_end(ap2);
    return r;
}
