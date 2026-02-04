/*
 * Copyright (c) 2026 Trollycat
 * Kernel entry.
 */

#include <thuban/stdio.h>
#include <thuban/multiboot.h>
#include <thuban/pmm.h>
#include <thuban/vmm.h>
#include <thuban/paging.h>
#include <thuban/heap.h>
#include <thuban/gdt.h>
#include <thuban/idt.h>
#include <thuban/interrupts.h>
#include <thuban/shell.h>
#include <thuban/module.h>
#include <thuban/syscall.h>
#include <thuban/blkdev.h>
#include <thuban/vfs.h>
#include <thuban/fat32.h>

static void create_directory_structure(void)
{
    const char *sys_dirs[] = {
        "/bin", "/boot", "/dev", "/etc", "/lib", "/media", "/mnt",
        "/opt", "/proc", "/root", "/run", "/sbin", "/srv", "/sys",
        "/tmp", "/usr", "/var", "/home", NULL};

    for (int i = 0; sys_dirs[i]; i++)
    {
        vfs_mkdir(sys_dirs[i], 0555);
    }

    const char *usr_dirs[] = {
        "/usr/bin", "/usr/lib", "/usr/local", "/usr/sbin", "/usr/share", NULL};
    for (int i = 0; usr_dirs[i]; i++)
    {
        vfs_mkdir(usr_dirs[i], 0555);
    }

    const char *var_dirs[] = {
        "/var/log", "/var/tmp", "/var/cache", NULL};
    for (int i = 0; var_dirs[i]; i++)
    {
        vfs_mkdir(var_dirs[i], 0555);
    }

    vfs_mkdir("/home/user", 0755);

    const char *user_dirs[] = {
        "/home/user/Desktop", "/home/user/Videos", "/home/user/Documents",
        "/home/user/Downloads", "/home/user/Music", "/home/user/Pictures", NULL};
    for (int i = 0; user_dirs[i]; i++)
    {
        vfs_mkdir(user_dirs[i], 0755);
    }
}

void kmain(uint32_t multiboot_magic, void *multiboot_addr)
{
    multiboot_parse(multiboot_magic, multiboot_addr);
    struct multiboot_info *mbi = multiboot_get_info();

    pmm_init(mbi->total_mem);
    paging_init();
    vmm_init();
    heap_init();
    gdt_init();
    idt_init();
    interrupts_init();
    blkdev_init();
    module_init_builtin();
    interrupts_enable();
    syscall_init();
    vfs_init();
    fat32_init();

    if (vfs_mount("hda", "/", "fat32", 0) == 0)
    {
        create_directory_structure();
        vfs_node_t *user_home = vfs_resolve_path("/home/user");
        if (user_home)
            vfs_set_cwd(user_home);
    }

    shell_init();
    shell_run();

    while (1)
        asm volatile("hlt");
}
