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

/*
 * Entry point method responsible for initializing all kernel systems
 * This is called by the bootloader assembly file
 * NOTE: multiboot_magic and multiboot_addr are passed from boot.s
 */
void kmain(uint32_t multiboot_magic, void *multiboot_addr)
{
    vga_init();

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

    keyboard_init();
    interrupts_enable();

    // Shell
    shell_init();
    shell_run();

    while (1)
    {
        asm volatile("hlt");
    }
}