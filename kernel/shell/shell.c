/*
 * Copyright (c) 2026 Trollycat
 * Shell implementation
 */

#include <thuban/shell.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/io.h>
#include <thuban/vga.h>
#include <thuban/pmm.h>
#include <thuban/heap.h>
#include <thuban/module.h>
#include <thuban/multiboot.h>
#include <thuban/panic.h>
#include <thuban/blkdev.h>
#include <thuban/vfs.h>

#define MAX_COMMAND_LEN 256
#define MAX_ARGS 16

static char command_buffer[MAX_COMMAND_LEN];

/* Forward declarations for VFS commands */
static void cmd_mount(int argc, char **argv);
static void cmd_ls(int argc, char **argv);
static void cmd_cat(int argc, char **argv);
static void cmd_mkdir(int argc, char **argv);
static void cmd_touch(int argc, char **argv);
static void cmd_write(int argc, char **argv);
static void cmd_rm(int argc, char **argv);
static void cmd_rmdir(int argc, char **argv);
static void cmd_cd(int argc, char **argv);
static void cmd_pwd(int argc, char **argv);

/*
 * Split's command into arguments
 */
static int parse_command(char *cmd, char **args)
{
    int argc = 0;
    char *token = cmd;

    while (*token && argc < MAX_ARGS)
    {
        // skip whitespace
        while (*token == ' ' || *token == '\t')
        {
            token++;
        }

        if (*token == '\0')
        {
            break;
        }

        args[argc++] = token;

        // find end of token
        while (*token && *token != ' ' && *token != '\t')
        {
            token++;
        }

        if (*token)
        {
            *token++ = '\0';
        }
    }

    return argc;
}

/* Helper: build the current working directory path string into buf.
 * Returns buf on success. */
static char *get_cwd_string(char *buf, int buflen)
{
    vfs_node_t *cwd = vfs_get_cwd();
    if (!cwd)
    {
        buf[0] = '/';
        buf[1] = '\0';
        return buf;
    }

    const char *parts[64];
    int depth = 0;

    vfs_node_t *n = cwd;
    while (n && n->parent && depth < 63)
    {
        parts[depth++] = n->name;
        n = n->parent;
    }

    if (depth == 0)
    {
        buf[0] = '/';
        buf[1] = '\0';
        return buf;
    }

    /* Build path root-first */
    int pos = 0;
    buf[pos++] = '/';
    for (int i = depth - 1; i >= 0; i--)
    {
        int len = strlen(parts[i]);
        if (pos + len + 1 >= buflen)
            break;
        memcpy(buf + pos, parts[i], len);
        pos += len;
        if (i > 0)
            buf[pos++] = '/';
    }
    buf[pos] = '\0';
    return buf;
}

/*
 * Command: help
 */
static void cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Thuban OS Shell - Available Commands:\n");
    printf("  help                - Display this help message\n");
    printf("  clear               - Clear the screen\n");
    printf("  meminfo             - Display memory information\n");
    printf("  sysinfo             - Display system information\n");
    printf("  drivers             - List all drivers\n");
    printf("  echo                - Echo arguments\n");
    printf("  reboot              - Reboot the system\n");
    printf("  panic               - Trigger a BSOD\n");
    printf("  lsblk               - List block devices\n");
    printf("  disktest            - Test disk read\n");
    printf("  diskwrite           - Test disk write\n");
    printf("  mount               - Mount a filesystem\n");
    printf("  ls [path]           - List directory contents\n");
    printf("  cd [path]           - Change directory\n");
    printf("  pwd                 - Print working directory\n");
    printf("  cat <file>          - Display file contents\n");
    printf("  mkdir <dir>         - Create directory\n");
    printf("  touch <file>        - Create empty file\n");
    printf("  write <file> <text> - Write text to file\n");
    printf("  rm <file>           - Remove file\n");
    printf("  rmdir <dir>         - Remove empty directory\n");
}

/*
 * Command: clear
 */
static void cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    vga_clear_screen();
    vga_set_cursor_pos(0, 0);
    terminal_reset();
}

/*
 * Command: meminfo
 */
static void cmd_meminfo(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uint64_t total_mem = pmm_get_total_memory();
    uint64_t used_mem = pmm_get_used_memory();
    uint64_t free_mem = pmm_get_free_memory();

    printf("Physical Memory:\n");
    printf("  Total: %llu MB (%llu KB)\n", total_mem / 1024 / 1024, total_mem / 1024);
    printf("  Used:  %llu MB (%llu KB)\n", used_mem / 1024 / 1024, used_mem / 1024);
    printf("  Free:  %llu MB (%llu KB)\n", free_mem / 1024 / 1024, free_mem / 1024);

    uint64_t heap_total = heap_get_total();
    uint64_t heap_used = heap_get_used();
    uint64_t heap_free = heap_get_free();

    printf("\nHeap Memory:\n");
    printf("  Total: %llu KB\n", heap_total / 1024);
    printf("  Used:  %llu KB\n", heap_used / 1024);
    printf("  Free:  %llu KB\n", heap_free / 1024);
}

/*
 * Command: sysinfo
 */
static void cmd_sysinfo(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct multiboot_info *mbi = multiboot_get_info();

    puts("[NAME]: Thuban");
    puts("[VERSION]: 0.3.0");
}

static void cmd_drivers(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    module_list();
}

/*
 * Command: echo
 */
static void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s", argv[i]);
        if (i < argc - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
}

/*
 * Command: reboot
 */
static void cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // use keyboard controller to reboot
    uint8_t temp;
    asm volatile("cli");

    do
    {
        temp = inb(0x64);
        if (temp & 0x01)
        {
            inb(0x60);
        }
    } while (temp & 0x02);

    outb(0x64, 0xFE);

    // if that didn't work try triple fault
    asm volatile("lidt (0)");
}

/*
 * Command: panic
 */
static void cmd_panic(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    panic(PANIC_MANUALLY_INITIATED_CRASH, "Panic command executed.");
}

/*
 * Command: lsblk
 */
static void cmd_blkdev_list(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    blkdev_list();
}

/*
 * Command: disktest
 */
static void cmd_disktest(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct block_device *dev = blkdev_find("hda");
    if (!dev)
    {
        printf("No disk 'hda' found\n");
        printf("Available devices:\n");
        blkdev_list();
    }
    else
    {
        char buffer[512];

        if (blkdev_read(dev, 0, 1, buffer) < 0)
        {
            printf("ERROR: Read failed!\n");
        }
        else
        {
            for (int i = 0; i < 64; i++)
            {
                printf("%02x ", (unsigned char)buffer[i]);
                if ((i + 1) % 16 == 0)
                    printf("\n");
            }
            printf("\n");

            // Check for boot signature
            if ((unsigned char)buffer[510] == 0x55 &&
                (unsigned char)buffer[511] == 0xAA)
            {
                printf("Boot signature found: 0x55AA\n");
            }
            else
            {
                printf("No boot signature (FAT32 filesystem)\n");
            }
        }
    }
}

/*
 * Command: diskwrite
 */
static void cmd_diskwrite(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct block_device *dev = blkdev_find("hda");
    if (!dev)
    {
        printf("No disk 'hda' found\n");
    }
    else
    {
        char buffer[512];

        // Fill with test pattern
        for (int i = 0; i < 512; i++)
        {
            buffer[i] = (i % 256);
        }

        if (blkdev_write(dev, 1, 1, buffer) < 0)
        {
            printf("ERROR: Write failed!\n");
        }
        else
        {
            // Read it back
            memset(buffer, 0, 512);
            if (blkdev_read(dev, 1, 1, buffer) < 0)
            {
                printf("ERROR: Read failed!\n");
            }
            else
            {
                for (int i = 0; i < 32; i++)
                {
                    printf("%02x ", (unsigned char)buffer[i]);
                    if ((i + 1) % 16 == 0)
                        printf("\n");
                }

                // Verify pattern
                int errors = 0;
                for (int i = 0; i < 512; i++)
                {
                    if ((unsigned char)buffer[i] != (i % 256))
                    {
                        errors++;
                    }
                }
            }
        }
    }
}

/*
 * VFS/Filesystem Commands
 */

/*
 * Command: mount
 */
static void cmd_mount(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Usage: mount <device> <mountpoint> <fstype>\n");
        printf("Example: mount hda / fat32\n");
        return;
    }

    const char *device = argv[1];
    const char *mountpoint = argv[2];
    const char *fstype = argv[3];

    if (vfs_mount(device, mountpoint, fstype, 0) == 0)
    {
        printf("Mounted %s on %s (type: %s)\n", device, mountpoint, fstype);
    }
    else
    {
        printf("Failed to mount %s\n", device);
    }
}

/*
 * Command: ls
 */
static void cmd_ls(int argc, char **argv)
{
    const char *path;
    char display_path[256];

    if (argc >= 2)
    {
        path = argv[1];
        strncpy(display_path, argv[1], sizeof(display_path) - 1);
        display_path[sizeof(display_path) - 1] = '\0';
    }
    else
    {
        /* No argument — list cwd */
        path = ".";
        get_cwd_string(display_path, sizeof(display_path));
    }

    int fd = vfs_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0)
    {
        printf("ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    struct dirent dirents[16];
    int count;

    printf("Directory listing of %s:\n", display_path);

    while ((count = vfs_readdir(fd, dirents, 16)) > 0)
    {
        for (int i = 0; i < count; i++)
        {
            char type = (dirents[i].d_type == VFS_DIRECTORY) ? 'd' : 'f';
            printf("  [%c] %s\n", type, dirents[i].d_name);
        }
    }

    vfs_close(fd);
}

/*
 * Command: cat
 */
static void cmd_cat(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: cat <filename>\n");
        return;
    }

    const char *filename = argv[1];

    /* Reject flag-like arguments */
    if (filename[0] == '-')
    {
        printf("cat: invalid option '%s'\n", filename);
        return;
    }

    /* Resolve the path first so we can give a proper error for dirs */
    vfs_node_t *node = vfs_resolve_path(filename);
    if (!node)
    {
        printf("cat: '%s': No such file or directory\n", filename);
        return;
    }

    if (vfs_is_directory(node))
    {
        printf("cat: '%s': Is a directory\n", filename);
        return;
    }

    int fd = vfs_open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
        printf("cat: cannot open '%s'\n", filename);
        return;
    }

    char buffer[512];
    ssize_t bytes_read;

    while ((bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }

    printf("\n");
    vfs_close(fd);
}

/*
 * Command: mkdir
 */
static void cmd_mkdir(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: mkdir <dirname>\n");
        return;
    }

    const char *dirname = argv[1];

    if (dirname[0] == '-')
    {
        printf("mkdir: invalid option '%s'\n", dirname);
        return;
    }

    /* Check if it already exists so we can give a better message */
    vfs_node_t *existing = vfs_resolve_path(dirname);
    if (existing)
    {
        printf("mkdir: cannot create directory '%s': already exists\n", dirname);
        return;
    }

    if (vfs_mkdir(dirname, 0755) == 0)
    {
        printf("Directory '%s' created\n", dirname);
    }
    else
    {
        printf("mkdir: cannot create directory '%s'\n", dirname);
    }
}

/*
 * Command: touch
 */
static void cmd_touch(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: touch <filename>\n");
        return;
    }

    const char *filename = argv[1];

    if (filename[0] == '-')
    {
        printf("touch: invalid option '%s'\n", filename);
        return;
    }

    /* If it already exists, touch succeeds silently (no-op) */
    vfs_node_t *existing = vfs_resolve_path(filename);
    if (existing)
    {
        /* File or directory already exists — touch is a no-op */
        return;
    }

    int fd = vfs_open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd < 0)
    {
        printf("touch: cannot create '%s'\n", filename);
        return;
    }

    vfs_close(fd);
}

/*
 * Command: write
 */
static void cmd_write(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: write <filename> <text>\n");
        return;
    }

    const char *filename = argv[1];
    const char *text = argv[2];

    if (filename[0] == '-')
    {
        printf("write: invalid option '%s'\n", filename);
        return;
    }

    int fd = vfs_open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0)
    {
        printf("write: cannot open '%s'\n", filename);
        return;
    }

    ssize_t written = vfs_write(fd, text, strlen(text));
    if (written > 0)
    {
        printf("Wrote %d bytes to '%s'\n", (int)written, filename);
    }
    else
    {
        printf("write: failed to write to '%s'\n", filename);
    }

    vfs_close(fd);
}

/*
 * Command: cd
 */
static void cmd_cd(int argc, char **argv)
{
    /* Target defaults to "/" if no arg given */
    const char *target = (argc >= 2) ? argv[1] : "/";

    vfs_node_t *node = vfs_resolve_path(target);
    if (!node)
    {
        printf("cd: %s: No such file or directory\n", target);
        return;
    }

    if (!vfs_is_directory(node))
    {
        printf("cd: %s: Not a directory\n", target);
        return;
    }

    vfs_set_cwd(node);
}

/*
 * Command: pwd
 */
static void cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    char path[256];
    get_cwd_string(path, sizeof(path));
    printf("%s\n", path);
}

/*
 * Command: rm
 *
 * BUG FIX: old code just said "cannot remove" for everything.  Now
 * we check: if the target is a directory we tell the user to use
 * rmdir instead.  Also rejects '-' args.
 */
static void cmd_rm(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: rm <filename>\n");
        return;
    }

    const char *filename = argv[1];

    if (filename[0] == '-')
    {
        printf("rm: invalid option '%s'\n", filename);
        return;
    }

    /* Check if it's a directory and give a helpful message */
    vfs_node_t *node = vfs_resolve_path(filename);
    if (!node)
    {
        printf("rm: cannot remove '%s': No such file or directory\n", filename);
        return;
    }

    if (vfs_is_directory(node))
    {
        printf("rm: cannot remove '%s': Is a directory (use rmdir)\n", filename);
        return;
    }

    if (vfs_unlink(filename) == 0)
    {
        printf("File '%s' removed\n", filename);
    }
    else
    {
        printf("rm: cannot remove '%s'\n", filename);
    }
}

/*
 * Command: rmdir
 */
static void cmd_rmdir(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: rmdir <dirname>\n");
        return;
    }

    const char *dirname = argv[1];

    if (dirname[0] == '-')
    {
        printf("rmdir: invalid option '%s'\n", dirname);
        return;
    }

    vfs_node_t *node = vfs_resolve_path(dirname);
    if (!node)
    {
        printf("rmdir: cannot remove '%s': No such file or directory\n", dirname);
        return;
    }

    if (!vfs_is_directory(node))
    {
        printf("rmdir: cannot remove '%s': Not a directory (use rm)\n", dirname);
        return;
    }

    if (vfs_rmdir(dirname) == 0)
    {
        printf("Directory '%s' removed\n", dirname);
    }
    else
    {
        printf("rmdir: cannot remove '%s': Directory not empty\n", dirname);
    }
}

/*
 * Execute's a command
 */
static void execute_command(char *cmd)
{
    char *args[MAX_ARGS];
    int argc = parse_command(cmd, args);

    if (argc == 0)
    {
        return;
    }

    // System commands
    if (strcmp(args[0], "help") == 0)
    {
        cmd_help(argc, args);
    }
    else if (strcmp(args[0], "clear") == 0)
    {
        cmd_clear(argc, args);
    }
    else if (strcmp(args[0], "meminfo") == 0)
    {
        cmd_meminfo(argc, args);
    }
    else if (strcmp(args[0], "sysinfo") == 0)
    {
        cmd_sysinfo(argc, args);
    }
    else if (strcmp(args[0], "drivers") == 0)
    {
        cmd_drivers(argc, args);
    }
    else if (strcmp(args[0], "echo") == 0)
    {
        cmd_echo(argc, args);
    }
    else if (strcmp(args[0], "reboot") == 0)
    {
        cmd_reboot(argc, args);
    }
    else if (strcmp(args[0], "panic") == 0)
    {
        cmd_panic(argc, args);
    }
    // Storage commands
    else if (strcmp(args[0], "lsblk") == 0)
    {
        cmd_blkdev_list(argc, args);
    }
    else if (strcmp(args[0], "disktest") == 0)
    {
        cmd_disktest(argc, args);
    }
    else if (strcmp(args[0], "diskwrite") == 0)
    {
        cmd_diskwrite(argc, args);
    }
    // Filesystem commands
    else if (strcmp(args[0], "mount") == 0)
    {
        cmd_mount(argc, args);
    }
    else if (strcmp(args[0], "ls") == 0)
    {
        cmd_ls(argc, args);
    }
    else if (strcmp(args[0], "cat") == 0)
    {
        cmd_cat(argc, args);
    }
    else if (strcmp(args[0], "mkdir") == 0)
    {
        cmd_mkdir(argc, args);
    }
    else if (strcmp(args[0], "touch") == 0)
    {
        cmd_touch(argc, args);
    }
    else if (strcmp(args[0], "write") == 0)
    {
        cmd_write(argc, args);
    }
    else if (strcmp(args[0], "rm") == 0)
    {
        cmd_rm(argc, args);
    }
    else if (strcmp(args[0], "rmdir") == 0)
    {
        cmd_rmdir(argc, args);
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        cmd_cd(argc, args);
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        cmd_pwd(argc, args);
    }
    else
    {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands\n");
    }
}

/*
 * Initialize's the shell
 */
void shell_init(void)
{
    vga_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("\nWelcome to Thuban OS v0.3.0\n");
    printf("Type 'help' for available commands\n\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}

/*
 * Run's the shell main loop.
 * Prompt now shows the cwd so the user gets feedback from cd.
 */
void shell_run(void)
{
    while (1)
    {
        char cwd[256];
        get_cwd_string(cwd, sizeof(cwd));

        vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("%s $ ", cwd);
        vga_set_color(COLOR_WHITE, COLOR_BLACK);

        if (fgets(command_buffer, MAX_COMMAND_LEN))
        {
            execute_command(command_buffer);
        }
    }
}
