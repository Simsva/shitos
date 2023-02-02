#ifndef ANSI_H_
#define ANSI_H_

#define SGR(s) "\033[" #s "m"
#define SGR2(s) "\033[" s "m"

#define ANSI_RESET             SGR()

#define ANSI_FG_BLACK          SGR(30)
#define ANSI_FG_RED            SGR(31)
#define ANSI_FG_GREEN          SGR(32)
#define ANSI_FG_YELLOW         SGR(33)
#define ANSI_FG_BLUE           SGR(34)
#define ANSI_FG_MAGENTA        SGR(35)
#define ANSI_FG_CYAN           SGR(36)
#define ANSI_FG_WHITE          SGR(37)
#define ANSI_FG_8BIT(n)        SGR2("38;5;" #n)
#define ANSI_FG_24BIT(r, g, b) SGR2("38;2;" #r ";" #g ";" #b)
#define ANSI_FG_DEFAULT        SGR(39)

#define ANSI_BG_BLACK          SGR(40)
#define ANSI_BG_RED            SGR(41)
#define ANSI_BG_GREEN          SGR(42)
#define ANSI_BG_YELLOW         SGR(43)
#define ANSI_BG_BLUE           SGR(44)
#define ANSI_BG_MAGENTA        SGR(45)
#define ANSI_BG_CYAN           SGR(46)
#define ANSI_BG_WHITE          SGR(47)
#define ANSI_BG_8BIT(n)        SGR2("48;5;" #n)
#define ANSI_BG_24BIT(r, g, b) SGR2("48;2;" #r ";" #g ";" #b)
#define ANSI_BG_DEFAULT        SGR(49)

#define ANSI_FG_BRIGHT_BLACK   SGR(90)
#define ANSI_FG_BRIGHT_RED     SGR(91)
#define ANSI_FG_BRIGHT_GREEN   SGR(92)
#define ANSI_FG_BRIGHT_YELLOW  SGR(93)
#define ANSI_FG_BRIGHT_BLUE    SGR(94)
#define ANSI_FG_BRIGHT_MAGENTA SGR(95)
#define ANSI_FG_BRIGHT_CYAN    SGR(96)
#define ANSI_FG_BRIGHT_WHITE   SGR(97)

#define ANSI_BG_BRIGHT_BLACK   SGR(100)
#define ANSI_BG_BRIGHT_RED     SGR(101)
#define ANSI_BG_BRIGHT_GREEN   SGR(102)
#define ANSI_BG_BRIGHT_YELLOW  SGR(103)
#define ANSI_BG_BRIGHT_BLUE    SGR(104)
#define ANSI_BG_BRIGHT_MAGENTA SGR(105)
#define ANSI_BG_BRIGHT_CYAN    SGR(106)
#define ANSI_BG_BRIGHT_WHITE   SGR(107)

#endif // ANSI_H_
