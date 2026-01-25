/*
 * Copyright (c) 2026 Trollycat.
 * Main file containing kernel entry point.
 */

#include <thuban/vga.h>
#include <thuban/stdio.h>

/*
 * Entry point method, responsible for initalizing all kernel system's.
 * This Is called by the bootloader assembly file.
 */
void kmain(void)
{
    vga_init();

    /*
     * For now we display some fake information.
     */
    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    puts("[ OK ] Initalizing Kernel...");
    puts("[ OK ] Initalizing Memory...");
    puts("[ OK ] Initalizing Hardware...");
    puts("[ OK ] Initalizing Drivers...");
    puts("[ OK ] Initalizing Libraries...");
    puts("[ OK ] Initalizing Timer...");
    puts("[ OK ] Initalizing Shell...");
    puts("[ OK ] Initalizing Text...");

    vga_set_color(COLOR_LIGHT_GREEN | COLOR_LIGHT_RED, COLOR_BLACK);
    puts("\n--- System Information: ---");
    puts("STATUS: OK");

    while (1)
    {
        asm volatile("hlt");
    }
}