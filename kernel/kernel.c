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
#include <thuban/vga.h>
#include <thuban/fat32.h>

static void create_directory_structure(void)
{
/* Helper macro to check and free nodes */
#define CHECK_AND_CREATE(path, mode)                        \
    do                                                      \
    {                                                       \
        vfs_node_t *node = vfs_resolve_path(path);          \
        if (node)                                           \
        {                                                   \
            /* Already exists - free the allocated node! */ \
            if (node->fs_data)                              \
                free(node->fs_data);                        \
            free(node);                                     \
        }                                                   \
        else                                                \
        {                                                   \
            vfs_mkdir(path, mode);                          \
        }                                                   \
    } while (0)

    /* System directories (read-only) */
    CHECK_AND_CREATE("/bin", 0555);
    CHECK_AND_CREATE("/boot", 0555);
    CHECK_AND_CREATE("/dev", 0555);
    CHECK_AND_CREATE("/etc", 0555);
    CHECK_AND_CREATE("/lib", 0555);
    CHECK_AND_CREATE("/media", 0555);
    CHECK_AND_CREATE("/mnt", 0555);
    CHECK_AND_CREATE("/opt", 0555);
    CHECK_AND_CREATE("/proc", 0555);
    CHECK_AND_CREATE("/root", 0555);
    CHECK_AND_CREATE("/run", 0555);
    CHECK_AND_CREATE("/sbin", 0555);
    CHECK_AND_CREATE("/srv", 0555);
    CHECK_AND_CREATE("/sys", 0555);
    CHECK_AND_CREATE("/tmp", 0555);
    CHECK_AND_CREATE("/usr", 0555);
    CHECK_AND_CREATE("/var", 0555);
    CHECK_AND_CREATE("/home", 0555);

    /* /usr subdirectories */
    CHECK_AND_CREATE("/usr/bin", 0555);
    CHECK_AND_CREATE("/usr/lib", 0555);
    CHECK_AND_CREATE("/usr/local", 0555);
    CHECK_AND_CREATE("/usr/sbin", 0555);
    CHECK_AND_CREATE("/usr/share", 0555);

    /* /var subdirectories */
    CHECK_AND_CREATE("/var/log", 0555);
    CHECK_AND_CREATE("/var/tmp", 0555);
    CHECK_AND_CREATE("/var/cache", 0555);

    /* User directories (writable) */
    CHECK_AND_CREATE("/home/user", 0755);
    CHECK_AND_CREATE("/home/user/Desktop", 0755);
    CHECK_AND_CREATE("/home/user/Videos", 0755);
    CHECK_AND_CREATE("/home/user/Documents", 0755);
    CHECK_AND_CREATE("/home/user/Downloads", 0755);
    CHECK_AND_CREATE("/home/user/Music", 0755);
    CHECK_AND_CREATE("/home/user/Pictures", 0755);

#undef CHECK_AND_CREATE
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

        /* Mark system initialization as complete - enables permission enforcement */
        vfs_set_init_complete();

        /* Set starting directory to /home/user */
        vfs_node_t *user_home = vfs_resolve_path("/home/user");
        if (user_home)
            vfs_set_cwd(user_home);
    }

    shell_init();

    while (1)
        asm volatile("hlt");
}