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

/*
 * Entry point method responsible for initializing all kernel systems
 * This is called by the bootloader assembly file
 * NOTE: multiboot_magic and multiboot_addr are passed from boot.s
 */
void kmain(uint32_t multiboot_magic, void *multiboot_addr)
{
    // NOTE: VGA is initialized via early_initcall, but we need it NOW
    // so we call it manually before anything else
    vga_init();

    vga_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("Thuban OS v0.2\n");
    printf("Copyright (c) 2026 Trollycat\n\n");

    vga_set_color(COLOR_WHITE, COLOR_BLACK);

    // parse multiboot info
    multiboot_parse(multiboot_magic, multiboot_addr);
    struct multiboot_info *mbi = multiboot_get_info();

    printf("\n");

    // initialize memory management
    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("[ OK ] ");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    printf("Initializing Memory Management\n");

    pmm_init(mbi->total_mem);
    paging_init();
    vmm_init();
    heap_init();

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("[ OK ] ");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    printf("Initializing Hardware\n");

    gdt_init();
    idt_init();
    interrupts_init();

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("[ OK ] ");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    printf("Initializing Drivers\n");

    keyboard_init();

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("[ OK ] ");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    printf("Initializing Shell\n");

    // enable interrupts
    interrupts_enable();

    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    printf("\n--- System Information ---\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);

    printf("Status: ");
    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("OK\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);

    if (mbi->bootloader_name)
    {
        printf("Bootloader: %s\n", mbi->bootloader_name);
    }

    // display memory info
    uint64_t total_mem = pmm_get_total_memory();
    uint64_t used_mem = pmm_get_used_memory();
    uint64_t free_mem = pmm_get_free_memory();

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

    printf("\nKernel Stack: 64 KB\n");

    // test heap allocation
    vga_set_color(COLOR_CYAN, COLOR_BLACK);
    printf("\n[DEBUG] Testing heap allocation\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);

    char *test1 = (char *)malloc(1024);
    if (test1)
    {
        printf("  Allocated 1 KB at 0x%llx\n", (uint64_t)test1);
        free(test1);
        printf("  Freed 1 KB\n");
    }

    char *test2 = (char *)malloc(4096);
    if (test2)
    {
        printf("  Allocated 4 KB at 0x%llx\n", (uint64_t)test2);
        free(test2);
        printf("  Freed 4 KB\n");
    }

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("\nKernel initialization complete\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);

    // launch shell
    shell_init();
    shell_run();

    // should never reach here
    while (1)
    {
        asm volatile("hlt");
    }
}