#ifndef CTYPE_H_
#define CTYPE_H_

#include <_cheader.h>
#include <features.h>

_BEGIN_C_HEADER

int isalnum(int);
int isalpha(int);
int isblank(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int isascii(int);

int tolower(int);
int toupper(int);
int toascii(int);

_END_C_HEADER

#endif // CTYPE_H_
