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

    // display memory info
    uint64_t total_mem = pmm_get_total_memory();
    uint64_t used_mem = pmm_get_used_memory();
    uint64_t free_mem = pmm_get_free_memory();

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("\nPhysical Memory:\n");
    printf("  Total: %llu MB\n", total_mem / 1024 / 1024);
    printf("  Used:  %llu MB (%llu KB)\n", used_mem / 1024 / 1024, used_mem / 1024);
    printf("  Free:  %llu MB\n", free_mem / 1024 / 1024);

    uint64_t heap_total = heap_get_total();
    uint64_t heap_used = heap_get_used();
    uint64_t heap_free = heap_get_free();

    printf("\nHeap Memory:\n");
    printf("  Total: %llu KB\n", heap_total / 1024);
    printf("  Used:  %llu KB\n", heap_used / 1024);
    printf("  Free:  %llu KB\n", heap_free / 1024);

    // Shell
    shell_init();
    shell_run();
    while (1)
    {
        asm volatile("hlt");
    }
}