ROOT=../../../../
include $(ROOT)/include/mk/def_flags.mk
include $(ROOT)/include/mk/programs.mk
CPPFLAGS+=-Iinclude -Iinclude_private
CFLAGS+=-ffreestanding

STAGE2=$(ROOT)/bin/stage2.bin
STAGE2_ELF=$(STAGE2:.bin=.elf)
STAGE2_SRC_ASM=$(wildcard *.asm)
STAGE2_SRC_C=$(wildcard *.c)
STAGE2_SRC_MAKE=$(STAGE2_SRC_C:.c=.c.d)
STAGE2_OBJ=$(STAGE2_SRC_ASM:.asm=.asm.o) $(STAGE2_SRC_C:.c=.c.o)
STAGE2_LD=link.ld

BUILDFILES=$(STAGE2_OBJ) $(STAGE2_SRC_MAKE) $(STAGE2) $(STAGE2_ELF)


all: stage2


clean:
	@echo Cleaning stage2
	@rm -f $(BUILDFILES)


include $(ROOT)/include/mk/compile.mk


stage2: $(STAGE2) $(STAGE2_ELF)
$(STAGE2): $(STAGE2_OBJ)
	@echo "LD	$(shell basename $(STAGE2))"
	@$(LD) -o $(STAGE2) $^ $(LDFLAGS) -T$(STAGE2_LD)
# used for debugging
$(STAGE2_ELF): $(STAGE2_OBJ)
	@echo "LD	$(shell basename $(STAGE2_ELF))"
	@$(LD) -o $(STAGE2_ELF) $^ $(LDFLAGS) -T$(STAGE2_LD) \
		--oformat=elf32-i386


install: install_headers
install_headers:
	@echo "Installing stage2 headers"
	@mkdir -p $(SYSROOT)/$(SYSINCLUDEDIR)
	@cp -R --preserve=timestamps include/. $(SYSROOT)/$(SYSINCLUDEDIR)/.


-include $(STAGE2_SRC_MAKE)


.PHONY: all clean stage2
