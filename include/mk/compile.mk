# default compilation targets

%.c.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)


%.asm.o: %.asm
	@echo "AS	$(shell basename $@)"
	@$(AS) -o $@ $< $(ASFLAGS)
