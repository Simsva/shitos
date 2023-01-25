include ../../include/mk/programs.mk
include ../../include/mk/def_flags.mk

KERNEL=../../bin/shitos.elf
KERNEL_DBG=../../bin/shitos_dbg.elf
KERNEL_SRC_ASM=$(wildcard *.asm)
KERNEL_SRC_C=$(wildcard *.c)
KERNEL_OBJ=$(KERNEL_SRC_ASM:.asm=_asm.o) $(KERNEL_SRC_C:.c=_c.o)
KERNEL_DBG_OBJ=$(KERNEL_SRC_ASM:.asm=_asm.o) $(KERNEL_SRC_C:.c=_c_dbg.o)
KERNEL_LD=shitos.ld

BUILDFILES=$(KERNEL_OBJ) $(KERNEL_DBG_OBJ) $(KERNEL) $(KERNEL_DBG)


all: kernel


clean:
	@echo Cleaning kernel
	rm -f $(BUILDFILES)


include ../../include/mk/compile.mk


kernel: $(KERNEL) $(KERNEL_DBG)
$(KERNEL): $(KERNEL_OBJ)
	@echo "LD	$(shell basename $(KERNEL))"
	@$(LD) -o $(KERNEL) $^ $(LDFLAGS) -T$(KERNEL_LD)
# used for debugging
$(KERNEL_DBG): $(KERNEL_DBG_OBJ)
	@echo "LD	$(shell basename $(KERNEL_DBG))"
	@$(LD) -o $(KERNEL_DBG) $^ $(LDFLAGS) -T$(KERNEL_LD)


.PHONY: all clean kernel
