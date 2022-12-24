OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
    . = 0x9000;

    .text : ALIGN(4K)
    {
        *(.text.prologue)
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

    end = .;
}
