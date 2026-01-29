/*
 * Copyright (c) 2026 Trollycat
 * Main file containing kernel entry point
 */

#include <thuban/vga.h>
#include <thuban/stdio.h>
#include <thuban/multiboot.h>
#include <thuban/pmm.h>
#include <thuban/vmm.h>
#include <thuban/paging.h>
#include <thuban/heap.h>
#include <thuban/gdt.h>
#include <thuban/idt.h>
#include <thuban/interrupts.h>
#include <thuban/keyboard.h>
#include <thuban/shell.h>
#include <thuban/module.h>
#include <thuban/syscall.h>
#include <thuban/blkdev.h>
#include <thuban/ata_pio.h>

/*
 * Entry point method responsible for initializing all kernel systems
 * This is called by the bootloader assembly file
 * NOTE: multiboot_magic and multiboot_addr are passed from boot.s
 */
void kmain(uint32_t multiboot_magic, void *multiboot_addr)
{
    // parse multiboot info
    multiboot_parse(multiboot_magic, multiboot_addr);
    struct multiboot_info *mbi = multiboot_get_info();

    // initialize memory management
    pmm_init(mbi->total_mem);
    paging_init();
    vmm_init();
    heap_init();

    // initialize hardware
    gdt_init();
    idt_init();
    interrupts_init();

    // initalize module's
    module_init_builtin();
    interrupts_enable();

    // initialize syscall subsystem (ready for future user programs)
    syscall_init();

    // initialize block device layer
    blkdev_init();

    // initialize storage drivers
    ata_pio_init();

    // launch shell
    shell_init();
    shell_run();

    // should never reach here
    while (1)
    {
        asm volatile("hlt");
    }
}