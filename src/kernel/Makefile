ROOT=../../
include $(ROOT)/include/mk/def_flags.mk
include $(ROOT)/include/mk/programs.mk
CFLAGS+=-ffreestanding
CPPFLAGS+=-Iinclude -D_ARCH=$(ARCH) -D_KERNEL

LDFLAGS+=-nostdlib -lk -lgcc

ARCHDIR=arch/$(ARCH)
KERNEL=$(ROOT)/bin/shitos.elf
KERNEL_SRC_ASM=$(wildcard *.asm) $(wildcard **/*.asm) $(wildcard $(ARCHDIR)/*.asm)
KERNEL_SRC_C=$(wildcard *.c) $(wildcard **/*.c) $(wildcard $(ARCHDIR)/*.c)
KERNEL_SRC_MAKE=$(KERNEL_SRC_C:.c=_c.d)
KERNEL_OBJ=$(KERNEL_SRC_ASM:.asm=_asm.o) $(KERNEL_SRC_C:.c=_c.o)
KERNEL_LD=$(ARCHDIR)/shitos.ld

BUILDFILES=$(KERNEL_OBJ) $(KERNEL_SRC_MAKE) $(KERNEL)


all: kernel


clean:
	@echo Cleaning kernel
	@rm -f $(BUILDFILES)


include $(ROOT)/include/mk/compile.mk


kernel: $(KERNEL)
$(KERNEL): $(KERNEL_OBJ)
	@echo "LD	$(shell basename $@)"
# NOTE: for some reason LD fails to link to libk
#	$(LD) -T$(KERNEL_LD) -o $@ $^ $(LDFLAGS)
	@$(CC) -T$(KERNEL_LD) -o $@ $(CFLAGS) $^ $(LDFLAGS)


install: install_headers install_kernel
install_headers:
	@echo "Installing kernel headers"
	@mkdir -p $(SYSROOT)/$(SYSINCLUDEDIR)
	@cp -R --preserve=timestamps include/. $(SYSROOT)/$(SYSINCLUDEDIR)/.
install_kernel: $(KERNEL)
	@echo "Installing kernel"
	@mkdir -p $(SYSROOT)
	@cp -R --preserve=timestamps $(KERNEL) $(SYSROOT)
# strip symbols from production kernel
	@$(STRIP) $(SYSROOT)/$(shell basename $(KERNEL))


-include $(KERNEL_SRC_MAKE)


.PHONY: all clean kernel
