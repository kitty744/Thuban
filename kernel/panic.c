/*
 * Copyright (c) 2026 Trollycat
 * Kernel panic and BSOD implementation
 */

#include <thuban/panic.h>
#include <thuban/vga.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/keyboard.h>
#include <thuban/io.h>
#include <stdarg.h>

/* BSOD color scheme */
#define BSOD_BG COLOR_BLUE
#define BSOD_FG COLOR_WHITE

static int bsod_row = 0;
static int bsod_started = 0;

/*
 * Initialize BSOD screen
 */
static void bsod_init(void)
{
    if (bsod_started)
        return;

    bsod_started = 1;

    /* Disable interrupts immediately */
    asm volatile("cli");

    /* Set blue background, white text */
    vga_set_color(BSOD_FG, BSOD_BG);
    vga_clear_screen();
    vga_disable_cursor();

    bsod_row = 0;
}

/*
 * Print a line to BSOD at current row
 */
static void bsod_print(const char *str)
{
    if (!bsod_started)
        bsod_init();

    if (bsod_row >= VGA_HEIGHT)
        return;

    size_t len = strlen(str);
    size_t x = 0;

    for (size_t i = 0; i < len && x < VGA_WIDTH; i++)
    {
        if (str[i] == '\n')
        {
            bsod_row++;
            x = 0;
            if (bsod_row >= VGA_HEIGHT)
                break;
            continue;
        }

        vga_write_cell(str[i], vga_get_color(), x, bsod_row);
        x++;
    }

    bsod_row++;
}

/*
 * Print formatted string to BSOD
 */
static void bsod_printf(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    bsod_print(buffer);
}

/*
 * Print centered text on BSOD
 */
static void bsod_print_center(const char *str)
{
    if (!bsod_started)
        bsod_init();

    if (bsod_row >= VGA_HEIGHT)
        return;

    size_t len = strlen(str);
    size_t start_x = (VGA_WIDTH - len) / 2;

    for (size_t i = 0; i < len && (start_x + i) < VGA_WIDTH; i++)
    {
        vga_write_cell(str[i], vga_get_color(), start_x + i, bsod_row);
    }

    bsod_row++;
}

/*
 * Draw a horizontal line
 */
static void bsod_draw_line(void)
{
    if (bsod_row >= VGA_HEIGHT)
        return;

    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        vga_write_cell('-', vga_get_color(), x, bsod_row);
    }

    bsod_row++;
}

/*
 * Print register dump
 */
static void bsod_print_registers(struct registers *regs)
{
    bsod_print("");
    bsod_print_center("REGISTER DUMP");
    bsod_draw_line();

    bsod_printf("RAX: 0x%016llx  RBX: 0x%016llx", regs->rax, regs->rbx);
    bsod_printf("RCX: 0x%016llx  RDX: 0x%016llx", regs->rcx, regs->rdx);
    bsod_printf("RSI: 0x%016llx  RDI: 0x%016llx", regs->rsi, regs->rdi);
    bsod_printf("RBP: 0x%016llx  RSP: 0x%016llx", regs->rbp, regs->rsp);
    bsod_printf("R8:  0x%016llx  R9:  0x%016llx", regs->r8, regs->r9);
    bsod_printf("R10: 0x%016llx  R11: 0x%016llx", regs->r10, regs->r11);
    bsod_printf("R12: 0x%016llx  R13: 0x%016llx", regs->r12, regs->r13);
    bsod_printf("R14: 0x%016llx  R15: 0x%016llx", regs->r14, regs->r15);
    bsod_print("");
    bsod_printf("RIP: 0x%016llx  CS:  0x%04llx", regs->rip, regs->cs);
    bsod_printf("RFLAGS: 0x%016llx  SS:  0x%04llx", regs->rflags, regs->ss);
    bsod_printf("Error Code: 0x%016llx", regs->err_code);
}

/*
 * Print stack trace
 */
static void bsod_print_stack_trace(uint64_t rbp)
{
    struct stack_frame *frame = (struct stack_frame *)rbp;
    int count = 0;

    bsod_print("");
    bsod_print_center("STACK TRACE");
    bsod_draw_line();

    /* Stack trace validation - check if rbp looks reasonable */
    if (rbp < 0xFFFFFFFF80000000ULL)
    {
        bsod_print("  [Stack trace unavailable - invalid base pointer]");
        return;
    }

    while (frame && count < MAX_STACK_FRAMES)
    {
        /* Basic validation */
        if ((uint64_t)frame < 0xFFFFFFFF80000000ULL)
            break;

        if (frame->rip == 0)
            break;

        bsod_printf("  [%d] RIP: 0x%016llx  RBP: 0x%016llx",
                    count, frame->rip, (uint64_t)frame);

        frame = frame->rbp;
        count++;
    }

    if (count == 0)
    {
        bsod_print("  [No valid stack frames found]");
    }
}

/*
 * Wait for any keypress and reboot
 */
static void bsod_wait_and_reboot(void)
{
    /* Print reboot message at bottom of screen */
    bsod_row = VGA_HEIGHT - 2;
    bsod_print("");
    bsod_print_center("Press any key to reboot...");

    /* Disable interrupts during wait */
    asm volatile("cli");

    /* Use keyboard driver's hardware polling function */
    /* This flushes the buffer and waits for a new keypress */
    keyboard_wait_for_keypress();

    /* Small delay after keypress detected */
    for (volatile int i = 0; i < 100000; i++)
        ;

    /* Reboot via keyboard controller */
    uint8_t temp;
    do
    {
        temp = inb(0x64);
        if (temp & 0x01)
            inb(0x60);
    } while (temp & 0x02);

    outb(0x64, 0xFE);

    /* If that fails, triple fault */
    asm volatile("int $0xFF");

    /* Halt as last resort */
    while (1)
    {
        asm volatile("hlt");
    }
}

/*
 * Main panic function
 */
void panic(uint32_t error_code, const char *fmt, ...)
{
    char buffer[512];
    va_list args;

    /* Disable interrupts */
    asm volatile("cli");

    /* Initialize BSOD */
    bsod_init();

    /* Print header */
    bsod_print("");
    bsod_print_center("*** STOP: A fatal system error has occurred ***");
    bsod_print("");
    bsod_draw_line();

    /* Print error code */
    bsod_printf("Error Code: 0x%08X", error_code);
    bsod_print("");

    /* Print formatted message */
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    bsod_print(buffer);
    bsod_print("");
    bsod_draw_line();

    /* Print technical info */
    bsod_print("");
    bsod_print("The system has been halted to prevent damage. If this is the first");
    bsod_print("time you've seen this error, restart your computer. If it appears");
    bsod_print("again, follow these steps:");
    bsod_print("");
    bsod_print("* Check to make sure any new hardware or software is properly installed.");
    bsod_print("* If this is a new installation, ask your hardware/software manufacturer");
    bsod_print("  for any updates you might need.");
    bsod_print("");
    bsod_print("Technical information:");
    bsod_printf("*** STOP: 0x%08X", error_code);

    /* Wait for keypress and reboot */
    bsod_wait_and_reboot();

    /* Should never reach here, but satisfy noreturn attribute */
    __builtin_unreachable();
}

/*
 * Panic from exception context with register dump
 */
void panic_from_exception(struct registers *regs, uint32_t error_code, const char *message)
{
    /* Disable interrupts */
    asm volatile("cli");

    /* Initialize BSOD */
    bsod_init();

    /* Print header */
    bsod_print("");
    bsod_print_center("*** STOP: A fatal system error has occurred ***");
    bsod_print("");
    bsod_draw_line();

    /* Print error code and message */
    bsod_printf("Error Code: 0x%08X", error_code);
    bsod_printf("Exception: %s", message);
    bsod_print("");

    /* Print registers */
    bsod_print_registers(regs);

    /* Print stack trace */
    bsod_print_stack_trace(regs->rbp);

    /* Wait for keypress and reboot */
    bsod_wait_and_reboot();

    /* Should never reach here, but satisfy noreturn attribute */
    __builtin_unreachable();
}

/*
 * Warning print (non-fatal)
 */
void warn_print(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    /* Save current color */
    uint8_t old_color = vga_get_color();

    /* Print in yellow */
    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    printf("[WARN] %s\n", buffer);

    /* Restore color */
    vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
}