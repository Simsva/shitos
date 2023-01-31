AS=nasm
AR=$(HOST)-ar
CC=$(HOST)-gcc --sysroot=$(SYSROOT)
LD=$(HOST)-ld --sysroot=$(SYSROOT)
STRIP=$(HOST)-strip

CC+=-isystem=$(SYSINCLUDEDIR)
