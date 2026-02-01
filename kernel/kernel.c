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
#include <thuban/vfs.h>
#include <thuban/fat32.h>

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

    // initialize block device layer
    blkdev_init();

    // initalize module's
    module_init_builtin();
    interrupts_enable();

    // initialize syscall subsystem (ready for future user programs)
    syscall_init();

    // initialize VFS
    vfs_init();

    // initialize FAT32 driver
    fat32_init();

    // Auto-mount root filesystem and create directory tree
    if (vfs_mount("hda", "/", "fat32", 0) == 0)
    {

        /* Create standard Linux-style directory tree.
         * Errors are silently ignored - dirs may already
         * exist on disk from a previous boot. */
        const char *dirs[] = {
            "/bin", "/boot", "/dev", "/etc", "/home",
            "/lib", "/media", "/mnt", "/opt", "/proc",
            "/root", "/run", "/sbin", "/srv", "/sys",
            "/tmp", "/usr", "/var", NULL};
        for (int i = 0; dirs[i]; i++)
        {
            vfs_mkdir(dirs[i], 0755);
        }

        vfs_mkdir("/usr/bin", 0755);
        vfs_mkdir("/usr/lib", 0755);
        vfs_mkdir("/usr/local", 0755);
        vfs_mkdir("/usr/sbin", 0755);
        vfs_mkdir("/usr/share", 0755);
        vfs_mkdir("/var/log", 0755);
        vfs_mkdir("/var/tmp", 0755);
        vfs_mkdir("/var/cache", 0755);
    }
    else
    {
        printf("[KERNEL] Warning: Failed to mount root filesystem\n");
        printf("[KERNEL] Use 'mount hda / fat32' manually\n");
    }

    // launch shell
    shell_init();
    shell_run();

    // should never reach here
    while (1)
    {
        asm volatile("hlt");
    }
}
