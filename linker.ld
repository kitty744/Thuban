ENTRY(start)

SECTIONS
{
    . = 1M;

    .boot :
    {
        *(.boot)
    }

    . += 0xFFFFFFFF80000000;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xFFFFFFFF80000000)
    {
        _code_start = .;
        *(.text)
        _code_end = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFFFF80000000)
    {
        _rodata_start = .;
        *(.rodata)
        _rodata_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xFFFFFFFF80000000)
    {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFFFF80000000)
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }

    . = ALIGN(4K);
    _kernel_end = .;
}