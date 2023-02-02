OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS
{
    . = 0x20000;
    .low.text : {
        _kernel_lowtext_start = .;
        *(.low.data)
        *(.low.text)
    }

    . += 0xc0000000;
    .text ALIGN(4K) : AT (ADDR(.text) - 0xc0000000)
    {
        _kernel_text_start = .;
        *(.text)
    }

    .rodata ALIGN(4K) : AT (ADDR(.rodata) - 0xc0000000)
    {
        _kernel_rodata_start = .;
        *(.rodata)
    }

    .data ALIGN(4K) : AT (ADDR(.data) - 0xc0000000)
    {
        _kernel_data_start = .;
        *(.data)
    }

    .bss ALIGN(4K) : AT (ADDR(.bss) - 0xc0000000)
    {
        _kernel_bss_start = .;
        *(.bss)
    }

    _kernel_end = .;
}
