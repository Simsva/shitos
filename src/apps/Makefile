ROOT=../../
include $(ROOT)/include/mk/def_flags.mk
include $(ROOT)/include/mk/programs.mk

APPS_MK=$(wildcard mk/*.mk)
APPS=$(patsubst mk/%.mk,$(SYSROOT)/$(EXEC_PREFIX)/bin/%,$(APPS_MK))

ALL_SRC_C=$(wildcard *.c)
ALL_SRC_ASM=$(wildcard *.asm)
ALL_OBJ=$(ALL_SRC_C:.c=_c.o) $(ALL_SRC_ASM:.asm=_asm.o)
ALL_SRC_MK=$(ALL_OBJ:.o=.d)

# XXX: apps do not automatically recompile when changing crts (or libc)
# CRTS=$(wildcard $(SYSROOT)/$(SYSLIBDIR)/crt*.o)

BINDIR=$(SYSROOT)/$(EXEC_PREFIX)/bin
DIRS=$(BINDIR)
BUILDFILES=$(ALL_OBJ) $(ALL_SRC_MK)


all: $(DIRS) $(SYSROOT)/bin $(APPS)


clean:
	@echo "Cleaning apps"
	@rm -f $(BUILDFILES)


dirs: $(DIRS)
$(DIRS):
	@echo "MKDIR	$@"
	@mkdir -p $@


# TODO: move this and add a similar target for lib
$(SYSROOT)/bin:
	@echo "LN	$@"
	@[ "$(BINDIR)" != "$(SYSROOT)/bin" ] && [ ! -e "$(SYSROOT)/bin" ] && \
		(cd $(SYSROOT) && ln -sf "$(EXEC_PREFIX:/%=%)/bin" bin)


include $(ROOT)/include/mk/compile.mk


-include $(APPS_MK)


.PHONY: all clean dirs
