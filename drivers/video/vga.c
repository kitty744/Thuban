/*
 * Copyright (c) 2026 Trollycat.
 * VGA driver with color support.
 */

#include <thuban/vga.h>

static uint16_t *buffer = (uint16_t *)VGA_BUFFER;
static uint8_t current_color = 0x0F;
static size_t cursor_x = 0;
static size_t cursor_y = 0;

static inline uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t make_color(uint8_t fg, uint8_t bg)
{
    return fg | (bg << 4);
}

void vga_init(void)
{
    current_color = make_color(COLOR_WHITE, COLOR_BLACK);
    cursor_x = 0;
    cursor_y = 0;
    vga_clear_screen();
    vga_enable_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg)
{
    current_color = make_color(fg, bg);
}

uint8_t vga_get_color(void)
{
    return current_color;
}

void vga_set_cursor_pos(size_t x, size_t y)
{
    if (x >= VGA_WIDTH)
        x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT)
        y = VGA_HEIGHT - 1;

    cursor_x = x;
    cursor_y = y;

    uint16_t pos = y * VGA_WIDTH + x;

    asm volatile("outb %%al, %%dx" ::"a"(0x0F), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" ::"a"((uint8_t)(pos & 0xFF)), "d"(0x3D5));
    asm volatile("outb %%al, %%dx" ::"a"(0x0E), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" ::"a"((uint8_t)((pos >> 8) & 0xFF)), "d"(0x3D5));
}

void vga_get_cursor_pos(size_t *x, size_t *y)
{
    if (x)
        *x = cursor_x;
    if (y)
        *y = cursor_y;
}

void vga_enable_cursor(void)
{
    asm volatile("outb %%al, %%dx" ::"a"(0x0A), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" ::"a"(0x00), "d"(0x3D5));
    asm volatile("outb %%al, %%dx" ::"a"(0x0B), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" ::"a"(0x0F), "d"(0x3D5));
}

void vga_disable_cursor(void)
{
    asm volatile("outb %%al, %%dx" ::"a"(0x0A), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" ::"a"(0x20), "d"(0x3D5));
}

void vga_write_cell(char c, uint8_t color, size_t x, size_t y)
{
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;

    size_t index = y * VGA_WIDTH + x;
    buffer[index] = vga_entry(c, color);
}

uint16_t vga_read_cell(size_t x, size_t y)
{
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return 0;

    size_t index = y * VGA_WIDTH + x;
    return buffer[index];
}

void vga_clear_screen(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_write_cell(' ', current_color, x, y);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_set_cursor_pos(0, 0);
}

void vga_scroll_up(void)
{
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            buffer[y * VGA_WIDTH + x] = buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        vga_write_cell(' ', current_color, x, VGA_HEIGHT - 1);
    }
}