# Thuban Standard Library — stdio

## NAME

stdio — Standard input/output facilities for the Thuban kernel

## SYNOPSIS

    #include <thuban/stdio.h>

## DESCRIPTION

The Thuban `stdio` subsystem provides low-level terminal input and output
facilities backed by the VGA text-mode driver. This implementation is intended
for kernel-space use and mirrors familiar C standard library behavior where
possible.

All output is written directly to VGA memory. Cursor position, scrolling, and
text wrapping are handled internally by the terminal logic.

Input-related functions are currently **stubbed**.

This implementation is **not thread-safe**.

---

## TERMINAL BEHAVIOR

- Fixed-size VGA text buffer (`VGA_WIDTH` × `VGA_HEIGHT`)
- Automatic line wrapping
- Automatic vertical scrolling
- Cursor position tracked internally
- Tabs are 4-space aligned
- Backspace clears the previous character

---

## BASIC OUTPUT FUNCTIONS

### putchar()

    int putchar(int c);

Writes a single character to the terminal.

#### Special characters

| Character | Behavior |
|----------|----------|
| `\n` | Newline (advance row, reset column) |
| `\r` | Carriage return (column = 0) |
| `\t` | Tab (aligns to next 4-space boundary) |
| `\b` | Backspace (erases previous character) |

#### Example

    putchar('A');
    putchar('\n');

---

### putc()

    int putc(int c);

Wrapper for `putchar()`. Provided for compatibility with standard C naming.

---

### puts()

    int puts(const char *s);

Writes a null-terminated string followed by a newline.

#### Return value

Returns the number of characters written (including the newline).
Returns `-1` if `s` is `NULL`.

#### Example

    puts("Hello, kernel!");

---

## FORMATTED OUTPUT

### printf()

    int printf(const char *format, ...);

Prints formatted output to the terminal. This function supports a subset of
standard C `printf` formatting and is intended for kernel debugging and logging.

Internally, this function forwards to `vprintf()`.

#### Example

    printf("PID=%d NAME=%s\n", pid, name);

---

### vprintf()

    int vprintf(const char *format, va_list args);

Core formatting engine responsible for parsing format strings and printing
formatted output.

Most users should prefer `printf()`.

---

## FORMAT STRING SYNTAX

Formatting begins with `%` and follows this structure:

    %[flags][width][.precision][length]specifier

---

### Conversion Specifiers

| Specifier | Description |
|----------|-------------|
| `%c` | Character |
| `%s` | String |
| `%d`, `%i` | Signed decimal integer |
| `%u` | Unsigned decimal integer |
| `%x` | Hexadecimal (lowercase) |
| `%X` | Hexadecimal (uppercase) |
| `%o` | Octal |
| `%p` | Pointer (`0x` + hex value) |
| `%%` | Literal percent sign |

---

### Flags

| Flag | Meaning |
|------|--------|
| `-` | Left-align output within field |
| `0` | Zero-pad numeric output |

---

### Width

Specifies the minimum field width.

    printf("%8d", 42);

---

### Precision

Introduced with `.` and used for strings or numbers.

    printf("%.5s", "kernel");
    printf("%.4d", 7);

---

### Length Modifiers

| Modifier | Meaning |
|---------|--------|
| `l` | long |
| `ll` | long long |
| `h`, `hh` | accepted but ignored |
| `z`, `t` | treated as `long` |

---

## POINTER OUTPUT

### %p

Pointers are printed as:

    0xXXXXXXXXXXXXXXXX

Always zero-padded to 16 hexadecimal digits.

---

## INPUT FUNCTIONS (UNIMPLEMENTED)

### getc()

    int getc(void);

Reads a single character from input. Currently always returns `-1`.

---

### getchar()

    int getchar(void);

Wrapper for `getc()`.

---

### gets()

    char *gets(char *s);

Reads a line of input into `s` until newline. Currently returns `NULL`.

---

### fgets()

    char *fgets(char *s, int size);

Reads up to `size - 1` characters into `s`. Currently returns `NULL`.

---

## STRING OUTPUT FUNCTIONS (UNIMPLEMENTED)

### sprintf()

    int sprintf(char *str, const char *format, ...);

Writes formatted output to a string buffer. Currently returns `0`.

---

### vsprintf()

    int vsprintf(char *str, const char *format, va_list args);

`va_list` variant of `sprintf()`.

---

### snprintf()

    int snprintf(char *str, size_t size, const char *format, ...);

Size-limited formatted string output. Currently returns `0`.

---

### vsnprintf()

    int vsnprintf(char *str, size_t size, const char *format, va_list args);

`va_list` variant of `snprintf()`.

---

## NOTES

- VGA-only output
- No buffering; output is immediate
- Designed for kernel debugging and early boot environments

