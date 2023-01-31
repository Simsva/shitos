# default compilation targets

%_c.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)


%_asm.o: %.asm
	@echo "AS	$(shell basename $@)"
	@$(AS) -o $@ $< $(ASFLAGS)
