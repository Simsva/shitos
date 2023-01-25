# default compilation targets

%_c.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CFLAGS)


%_asm.o: %.asm
	@echo "AS	$(shell basename $@)"
	@$(AS) -o $@ $< $(ASFLAGS)


%_c_dbg.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CFLAGS) -g
