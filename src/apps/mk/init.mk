# TODO: macro for this somehow (sed?)
$(SYSROOT)/$(EXEC_PREFIX)/bin/init: init_c.o
	@echo "LD	$(shell basename $@)"
	@$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)
