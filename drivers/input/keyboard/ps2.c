/*
 * Copyright (c) 2026 Trollycat
 * PS/2 Keyboard driver implementation
 */

#include <thuban/keyboard.h>
#include <thuban/interrupts.h>
#include <thuban/io.h>
#include <thuban/stdio.h>
#include <thuban/vga.h>
#include <thuban/module.h>
#include <thuban/device.h>

MODULE_AUTHOR("Trollycat");
MODULE_DESCRIPTION("PS/2 Keyboard Driver");
MODULE_LICENSE("MIT");
MODULE_VERSION("0.1");

// keyboard state
static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t capslock_active = 0;
static uint8_t extended_key = 0;

// keyboard buffer
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_buffer_start = 0;
static volatile int kb_buffer_end = 0;

// US QWERTY scancode to ASCII
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '};

// shifted characters
static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '};

/*
 * Add's character to keyboard buffer
 */
static void kb_buffer_add(char c)
{
    int next = (kb_buffer_end + 1) % KB_BUFFER_SIZE;

    if (next != kb_buffer_start)
    {
        kb_buffer[kb_buffer_end] = c;
        kb_buffer_end = next;
    }
}

/*
 * Handle's special arrow keys and extended keys
 * NOTE: Add special codes to buffer so shell can handle them
 */
static void handle_extended_key(uint8_t scancode)
{
    switch (scancode)
    {
    case KEY_LEFT:
        kb_buffer_add(0x1B); // ESC
        kb_buffer_add('[');
        kb_buffer_add('D'); // left arrow sequence
        break;
    case KEY_RIGHT:
        kb_buffer_add(0x1B); // ESC
        kb_buffer_add('[');
        kb_buffer_add('C'); // right arrow sequence
        break;
    case KEY_UP:
    case KEY_DOWN:
    case KEY_HOME:
    case KEY_END:
    case KEY_DELETE:
    case KEY_INSERT:
    case KEY_PGUP:
    case KEY_PGDN:
        // ignore these keys
        break;
    default:
        break;
    }
}

/*
 * Keyboard IRQ handler
 */
static void keyboard_irq_handler(struct registers *regs)
{
    (void)regs;

    uint8_t scancode = inb(KB_DATA_PORT);

    // check for extended key prefix
    if (scancode == 0xE0)
    {
        extended_key = 1;
        return;
    }

    // handle key release (bit 7 set)
    if (scancode & 0x80)
    {
        scancode &= 0x7F;

        // handle modifier key releases
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT)
        {
            shift_pressed = 0;
        }
        else if (scancode == KEY_LCTRL)
        {
            ctrl_pressed = 0;
        }
        else if (scancode == KEY_LALT)
        {
            alt_pressed = 0;
        }

        extended_key = 0;
        return;
    }

    // handle extended keys
    if (extended_key)
    {
        handle_extended_key(scancode);
        extended_key = 0;
        return;
    }

    // handle modifier keys
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT)
    {
        shift_pressed = 1;
        return;
    }

    if (scancode == KEY_LCTRL)
    {
        ctrl_pressed = 1;
        return;
    }

    if (scancode == KEY_LALT)
    {
        alt_pressed = 1;
        return;
    }

    if (scancode == KEY_CAPSLOCK)
    {
        capslock_active = !capslock_active;
        return;
    }

    // sanitize special keys that shouldn't print
    if (scancode == KEY_ESC || scancode == KEY_F1 || scancode == KEY_F2 ||
        scancode == KEY_F3 || scancode == KEY_F4 || scancode == KEY_F5 ||
        scancode == KEY_F6 || scancode == KEY_F7 || scancode == KEY_F8 ||
        scancode == KEY_F9 || scancode == KEY_F10 || scancode == KEY_F11 ||
        scancode == KEY_F12 || scancode == KEY_NUMLOCK || scancode == KEY_SCROLLLOCK)
    {
        return;
    }

    // convert scancode to ASCII
    if (scancode < sizeof(scancode_to_ascii))
    {
        char c;

        // apply shift or capslock
        if (shift_pressed)
        {
            c = scancode_to_ascii_shift[scancode];
        }
        else
        {
            c = scancode_to_ascii[scancode];

            // apply capslock to letters only
            if (capslock_active && c >= 'a' && c <= 'z')
            {
                c = c - 'a' + 'A';
            }
        }

        // handle ctrl combinations
        if (ctrl_pressed && c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 1; // ctrl+a = 0x01, ctrl+c = 0x03, etc
        }

        if (c != 0)
        {
            kb_buffer_add(c);
        }
    }
}

/*
 * Initialize's the keyboard driver
 */
void keyboard_init(void)
{
    // install keyboard IRQ handler
    irq_install_handler(1, keyboard_irq_handler);

    // clear keyboard buffer
    kb_buffer_start = 0;
    kb_buffer_end = 0;

    // enable keyboard IRQ (unmask IRQ1)
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~0x02; // clear bit 1
    outb(PIC1_DATA, mask);
}

/*
 * Get's character from keyboard buffer
 */
int keyboard_getchar(void)
{
    if (kb_buffer_start == kb_buffer_end)
    {
        return -1;
    }

    char c = kb_buffer[kb_buffer_start];
    kb_buffer_start = (kb_buffer_start + 1) % KB_BUFFER_SIZE;

    return c;
}

/*
 * Check's if key is available in buffer
 */
int keyboard_available(void)
{
    return kb_buffer_start != kb_buffer_end;
}

/*
 * Get's raw scancode from keyboard
 */
uint8_t keyboard_get_scancode(void)
{
    if (inb(KB_STATUS_PORT) & 0x01)
    {
        return inb(KB_DATA_PORT);
    }
    return 0;
}

/*
 * Flush's keyboard buffer
 */
void keyboard_flush(void)
{
    kb_buffer_start = 0;
    kb_buffer_end = 0;
}

/*
 * Driver initialization function
 */
static int __init keyboard_driver_init(void)
{
    keyboard_init();
    return 0;
}

device_initcall(keyboard_driver_init);