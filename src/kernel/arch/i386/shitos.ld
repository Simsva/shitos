OUTPUT_FORMAT("elf32-i386")
ENTRY(kmain)
SECTIONS
{
    . = 0x20000;
    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(.bss)
    }
}
