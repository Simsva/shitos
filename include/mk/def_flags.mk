ARCH?=i386
HOST?=i686-elf
SYSROOT?=$(ROOT)/sysroot/
PREFIX?=/usr
EXEC_PREFIX?=$(PREFIX)
SYSINCLUDEDIR?=$(PREFIX)/include
SYSLIBDIR?=$(EXEC_PREFIX)/lib

INCS+=
LIBS+=

CPPFLAGS?=
CPPFLAGS+=$(INCS)

CFLAGS?=-O2 -g
CFLAGS+=-Wall -Wextra -MD -std=c99
# CFLAGS+=-fno-stack-protector -ffreestanding

ASFLAGS?=
ASFLAGS+=-f elf32 -w+orphan-labels

LDFLAGS?=
LDFLAGS+=$(LIBS)
