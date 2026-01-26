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
#include <thuban/multiboot.h>

#define MAX_COMMAND_LEN 256
#define MAX_ARGS 16

static char command_buffer[MAX_COMMAND_LEN];

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

/*
 * Command: help
 */
static void cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Thuban OS Shell - Available Commands:\n");
    printf("  help     - Display this help message\n");
    printf("  clear    - Clear the screen\n");
    printf("  meminfo  - Display memory information\n");
    printf("  sysinfo  - Display system information\n");
    printf("  echo     - Echo arguments\n");
    printf("  reboot   - Reboot the system\n");
    printf("  halt     - Halt the system\n");
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

    printf("Thuban OS v0.1\n");

    if (mbi->bootloader_name)
    {
        printf("Bootloader: %s\n", mbi->bootloader_name);
    }

    printf("Total RAM: %llu MB\n", mbi->total_mem / 1024 / 1024);
    printf("Kernel Stack: 64 KB\n");
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

    printf("Rebooting system...\n");

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
 * Command: halt
 */
static void cmd_halt(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("System halted\n");

    while (1)
    {
        asm volatile("cli; hlt");
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
    else if (strcmp(args[0], "echo") == 0)
    {
        cmd_echo(argc, args);
    }
    else if (strcmp(args[0], "reboot") == 0)
    {
        cmd_reboot(argc, args);
    }
    else if (strcmp(args[0], "halt") == 0)
    {
        cmd_halt(argc, args);
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
    printf("\nWelcome to Thuban OS Shell\n");
    printf("Type 'help' for available commands\n\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}

/*
 * Run's the shell main loop
 */
void shell_run(void)
{
    while (1)
    {
        vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("$ ");
        vga_set_color(COLOR_WHITE, COLOR_BLACK);

        if (fgets(command_buffer, MAX_COMMAND_LEN))
        {
            execute_command(command_buffer);
        }
    }
}