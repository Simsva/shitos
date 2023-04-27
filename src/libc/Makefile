ROOT=../../
include $(ROOT)/include/mk/def_flags.mk
include $(ROOT)/include/mk/programs.mk
CFLAGS+=-ffreestanding
CPPFLAGS+=-Iinclude -Iinternal -D__is_libc
ASFLAGS+=

LIBK_CFLAGS=$(CFLAGS)
LIBK_CPPFLAGS=$(CPPFLAGS) -D__is_libk
LIBK_ASFLAGS=$(ASFLAGS)

ARCHDIR=arch/$(ARCH)
LIBC=$(ROOT)/bin/libc.a
LIBK=$(ROOT)/bin/libk.a
LIBC_SRC_C=$(wildcard **/*.c)
LIBC_SRC_MAKE=$(LIBC_SRC_C:.c=_c.d)
LIBK_SRC_MAKE=$(LIBC_SRC_MAKE:.d=.libk.d)

CRTS_ASM=$(wildcard $(ARCHDIR)/crt*.asm)
CRTS=$(CRTS_ASM:.asm=.o)

# FIXME: differentiate free from hosted objects
LIBC_OBJ=$(LIBC_SRC_C:.c=_c.o)
LIBK_OBJ=$(LIBC_OBJ:.o=.libk.o)

LIBS=$(LIBC) $(LIBK)
BUILDFILES=$(LIBC_OBJ) $(LIBK_OBJ) $(LIBC_SRC_MAKE) $(LIBK_SRC_MAKE) $(LIBS) $(CRTS)


all: $(BINARIES) $(CRTS)


clean:
	@echo Cleaning libc
	@rm -f $(BUILDFILES)


include $(ROOT)/include/mk/compile.mk


%_c.libk.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(LIBK_CPPFLAGS) $(LIBK_CFLAGS)


%_asm.libk.o: %.asm
	@echo "AS	$(shell basename $@)"
	@$(AS) -o $@ $< $(LIBK_ASFLAGS)


# XXX: jank for crts
%.o: %.asm
	@echo "AS	$(shell basename $@)"
	@$(AS) -o $@ $< $(ASFLAGS)


$(LIBC): $(LIBC_OBJ)
	@echo "AR	$(shell basename $@)"
	@$(AR) rcs $@ $(LIBC_OBJ)


$(LIBK): $(LIBK_OBJ)
	@echo "AR	$(shell basename $@)"
	@$(AR) rcs $@ $(LIBK_OBJ)


install: install_headers install_libs install_crts
install_headers:
	@echo "Installing libc headers"
	@mkdir -p $(SYSROOT)/$(SYSINCLUDEDIR)
	@cp -R --preserve=timestamps include/. $(SYSROOT)/$(SYSINCLUDEDIR)/.
install_libs: $(LIBS)
	@echo "Installing libc"
	@mkdir -p $(SYSROOT)/$(SYSLIBDIR)
	@cp -R --preserve=timestamps $(LIBS) $(SYSROOT)/$(SYSLIBDIR)
install_crts: $(CRTS)
	@echo "Installing crts"
	@mkdir -p $(SYSROOT)/$(SYSLIBDIR)
	@cp -R --preserve=timestamps $(CRTS) $(SYSROOT)/$(SYSLIBDIR)


-include $(LIBC_SRC_MAKE)


.PHONY: all clean install install_headers install_libs
