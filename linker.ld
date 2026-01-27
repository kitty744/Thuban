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
        *(.init.text)
        _code_end = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFFFF80000000)
    {
        _rodata_start = .;
        *(.rodata)
        
        /* module metadata section */
        . = ALIGN(8);
        __modinfo_start = .;
        *(.modinfo)
        __modinfo_end = .;
        
        _rodata_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xFFFFFFFF80000000)
    {
        _data_start = .;
        *(.data)
        *(.init.data)
        _data_end = .;
    }

    .init ALIGN(4K) : AT(ADDR(.init) - 0xFFFFFFFF80000000)
    {
        __init_start = .;
        
        /* initcall sections ordered by level */
        . = ALIGN(8);
        __initcall0_start = .;
        *(.initcall0.init)
        __initcall1_start = .;
        *(.initcall1.init)
        __initcall2_start = .;
        *(.initcall2.init)
        __initcall3_start = .;
        *(.initcall3.init)
        __initcall4_start = .;
        *(.initcall4.init)
        __initcall5_start = .;
        *(.initcall5.init)
        __initcall6_start = .;
        *(.initcall6.init)
        __initcall_end = .;
        
        __init_end = .;
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFFFF80000000)
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }

    .exit ALIGN(4K) : AT(ADDR(.exit) - 0xFFFFFFFF80000000)
    {
        __exit_start = .;
        *(.exit.text)
        *(.exit.data)
        __exit_end = .;
    }

    . = ALIGN(4K);
    _kernel_end = .;
}