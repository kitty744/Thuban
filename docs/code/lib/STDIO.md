# Stdio library documentation for Thuban

The **stdio** library provides high-level text output and formatting for the Thuban kernel, interacting directly with the VGA driver.

## 1. Output Methods

### putchar()

**Usage:** int putchar(int c);
**About:** Writes a character to the screen at (term_x, term_y).

- **Special Handling:**
  - \n : Moves to a new line; triggers terminal scroll if at the bottom.
  - \r : Resets horizontal position (term_x) to 0.
  - \t : Advances to the next 4-space tab stop.
  - \b : Moves back one space and overwrites the character with a blank cell.

### putc()

**Usage:** int putc(int c);
**About:** A functional wrapper for putchar().

### puts()

**Usage:** int puts(const char \*s);
**About:** Writes a null-terminated string followed by a newline.
**Example:**

```c
puts("Kernel initialized.");
```

---

## 2. Formatting (printf)

### printf() / vprintf()

**Usage:** int printf(const char \*format, ...);
**About:** Processes a format string to display variables.

**Supported Specifiers:**

- %d, %i : Signed integers (supports 'l' and 'll' modifiers).
- %u : Unsigned integers.
- %x, %X : Hexadecimal (lowercase/uppercase).
- %o : Octal.
- %s : Null-terminated strings.
- %c : Single characters.
- %p : Pointer addresses (auto-prefixes 0x and pads to 16 chars).
- %% : Prints a literal '%' sign.

**Formatting Flags:**

- '-' : Left-align within field width.
- '0' : Pad with zeros instead of spaces.
- Width : Define minimum character width (e.g., %10d).
- Precision : Define string limit or integer padding (e.g., %.5s).

---

## 3. Unfinished / Stubs

### Input Methods

These require a keyboard driver to be implemented. Currently, they return error values.

- **getc() / getchar()**: Will read a single character from input. Returns -1.
- **gets() / fgets()**: Will read a string from input. Returns NULL.

### String Buffering

These are stubs for formatting text into a memory buffer rather than the screen.

- **sprintf() / vsprintf()**: Formats output into a char buffer. Returns 0.
- **snprintf() / vsnprintf()**: Formats output into a buffer with a size limit. Returns 0.
