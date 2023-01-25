ROOT=../../../
include $(ROOT)/include/mk/programs.mk
include $(ROOT)/include/mk/def_flags.mk
CFLAGS+=-g

STAGE2=$(ROOT)/bin/stage2.bin
STAGE2_ELF=$(STAGE2:.bin=.elf)
STAGE2_SRC_ASM=$(wildcard *.asm)
STAGE2_SRC_C=$(wildcard *.c)
STAGE2_OBJ=$(STAGE2_SRC_ASM:.asm=_asm.o) $(STAGE2_SRC_C:.c=_c.o)
STAGE2_LD=link.ld

BUILDFILES=$(STAGE2_OBJ) $(STAGE2) $(STAGE2_ELF)


all: stage2


clean:
	@echo Cleaning stage2
	rm -f $(BUILDFILES)


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


.PHONY: all clean stage2
