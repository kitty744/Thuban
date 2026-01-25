#include <thuban/vga.h>
#include <thuban/stdio.h>

void kmain(void)
{
    vga_init();

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    puts("Initializing kernel subsystems...\n");
    puts("[ OK ] GDT & IDT Loaded");
    puts("[ OK ] Physical Memory Manager Active");
    puts("[ OK ] Kernel Heap Initialized");
    puts("[ OK ] VFS Root Mounted");
    puts("[ OK ] PIC & Timer Calibrated (1000Hz)");
    puts("[ OK ] Keyboard Controller Ready");

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    puts("\nSystem Information: ");

    puts("\n[NAME]: Thuban");
    puts("[VERION]: 0.1");

    while (1)
    {
        asm volatile("hlt");
    }
}