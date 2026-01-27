/*
 * Copyright (c) 2026 Trollycat
 * PS/2 Keyboard driver for Thuban
 */

#ifndef THUBAN_KEYBOARD_H
#define THUBAN_KEYBOARD_H

#include <stdint.h>

// keyboard ports
#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64
#define KB_COMMAND_PORT 0x64

// special keys
#define KEY_ESC 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ENTER 0x1C
#define KEY_LCTRL 0x1D
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LALT 0x38
#define KEY_CAPSLOCK 0x3A
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_NUMLOCK 0x45
#define KEY_SCROLLLOCK 0x46
#define KEY_F11 0x57
#define KEY_F12 0x58

// extended keys (0xE0 prefix)
#define KEY_UP 0x48
#define KEY_DOWN 0x50
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_HOME 0x47
#define KEY_END 0x4F
#define KEY_PGUP 0x49
#define KEY_PGDN 0x51
#define KEY_INSERT 0x52
#define KEY_DELETE 0x53

// keyboard buffer size
#define KB_BUFFER_SIZE 256

// initialize keyboard driver
void keyboard_init(void);

// get character from keyboard buffer
int keyboard_getchar(void);

// check if key is available
int keyboard_available(void);

// get raw scancode
uint8_t keyboard_get_scancode(void);

// flush keyboard buffer
void keyboard_flush(void);

/*
 * Wait for a keypress via hardware polling
 * Used by panic/BSOD system when interrupts are disabled
 * Returns the raw scancode of the pressed key
 */
int keyboard_wait_for_keypress(void);

#endif