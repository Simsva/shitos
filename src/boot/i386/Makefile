ROOT=../../../
include $(ROOT)/include/mk/def_flags.mk
include $(ROOT)/include/mk/programs.mk

MBR=$(ROOT)/bin/mbr.bin
MBR_ELF=$(MBR:.bin=.elf)
MBR_SRC=mbr.asm
MBR_OBJ=$(MBR_SRC:.asm=_asm.o)

STAGE1=$(ROOT)/bin/stage1.bin
STAGE1_ELF=$(STAGE1:.bin=.elf)
STAGE1_SRC=stage1.asm
STAGE1_OBJ=$(STAGE1_SRC:.asm=_asm.o)

BUILDFILES=$(MBR_OBJ) $(STAGE1_OBJ) $(MBR) $(MBR_ELF) $(STAGE1) $(STAGE1_ELF)


all: mbr stage1 stage2


clean:
	@echo Cleaning boot
	@rm -f $(BUILDFILES)

	@(cd $(ROOT)/src/boot/$(ARCH)/stage2 && $(MAKE) clean)


include $(ROOT)/include/mk/compile.mk


mbr: $(MBR) $(MBR_ELF)
$(MBR): $(MBR_OBJ)
	@echo "LD	$(shell basename $@)"
	@$(LD) -o $(MBR) $^ $(LDFLAGS) -Ttext 0x0 --oformat=binary
# used for debugging
$(MBR_ELF): $(MBR_OBJ)
	@echo "LD	$(shell basename $@)"
	@$(LD) -o $(MBR_ELF) $^ $(LDFLAGS) -Ttext 0x600


stage1: $(STAGE1) $(STAGE1_ELF)
$(STAGE1): $(STAGE1_OBJ)
	@echo "LD	$(shell basename $@)"
	@$(LD) -o $(STAGE1) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary
# used for debugging
$(STAGE1_ELF): $(STAGE1_OBJ)
	@echo "LD	$(shell basename $@)"
	@$(LD) -o $(STAGE1_ELF) $^ $(LDFLAGS) -Ttext 0x7c00


stage2:
	@(cd $(ROOT)/src/boot/$(ARCH)/stage2 && $(MAKE) stage2 install)
install_headers:
	@(cd $(ROOT)/src/boot/$(ARCH)/stage2 && $(MAKE) install_headers)


.PHONY: all clean mbr stage1 stage2
