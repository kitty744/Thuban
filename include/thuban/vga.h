#ifndef THUBAN_VGA_H
#define THUBAN_VGA_H

#include <stdint.h>
#include <stddef.h>

// vga colors
enum vga_color
{
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15,
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000

// low level vga hardware access
void vga_init(void);
void vga_set_color(uint8_t fg, uint8_t bg);
uint8_t vga_get_color(void);
void vga_set_cursor_pos(size_t x, size_t y);
void vga_get_cursor_pos(size_t *x, size_t *y);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_write_cell(char c, uint8_t color, size_t x, size_t y);
uint16_t vga_read_cell(size_t x, size_t y);
void vga_clear_screen(void);
void vga_scroll_up(void);

#endif