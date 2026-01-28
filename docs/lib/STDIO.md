# Standard I/O Library (stdio)

## Overview

The Thuban stdio library provides standard input/output functions for kernel and user-space programs. It includes character I/O, string I/O, and formatted output capabilities similar to standard C libraries.

## Table of Contents

1. [Character I/O](#character-io)
2. [String I/O](#string-io)
3. [Formatted Output](#formatted-output)
4. [Terminal Control](#terminal-control)
5. [Implementation Details](#implementation-details)

---

## Character I/O

### putchar()

```c
int putchar(int c);
```

**Description:**  
Writes a single character to the screen using the VGA driver.

**Parameters:**

- `c`: Character to write (as int)

**Returns:**  
The character written

**Special Characters:**

- `\n` - Newline (moves to start of next line)
- `\r` - Carriage return (moves to start of current line)
- `\t` - Tab (moves to next 4-character boundary)
- `\b` - Backspace (moves cursor left one position)

**Example:**

```c
putchar('A');        // Prints 'A'
putchar('\n');       // Moves to next line
putchar('\t');       // Inserts tab
```

---

### putc()

```c
int putc(int c);
```

**Description:**  
Wrapper for `putchar()`. Provided for compatibility.

**Parameters:**

- `c`: Character to write

**Returns:**  
The character written

**Example:**

```c
putc('X');  // Same as putchar('X')
```

---

### getchar()

```c
int getchar(void);
```

**Description:**  
Reads a single character from keyboard input. Blocks until a key is pressed.

**Returns:**  
The character read, or -1 on error

**Example:**

```c
printf("Press any key: ");
int c = getchar();
printf("You pressed: %c\n", c);
```

**Notes:**

- Uses keyboard driver circular buffer
- Halts CPU while waiting (energy efficient)
- Returns immediately if key is already in buffer

---

### getc()

```c
int getc(void);
```

**Description:**  
Wrapper for `getchar()`.

**Returns:**  
The character read

---

## String I/O

### puts()

```c
int puts(const char *s);
```

**Description:**  
Writes a string to the screen and appends a newline.

**Parameters:**

- `s`: Null-terminated string to print

**Returns:**  
Number of characters written (including newline), or -1 on error

**Example:**

```c
puts("Hello, World!");  // Prints "Hello, World!\n"
```

---

### gets()

```c
char *gets(char *s);
```

**Description:**  
Reads a line from keyboard input until Enter is pressed. Echoes characters as typed.

**Parameters:**

- `s`: Buffer to store input

**Returns:**  
Pointer to `s`, or NULL on error

**Features:**

- Echoes characters as typed
- Handles backspace
- Null-terminates string
- Stops at Enter key

**Example:**

```c
char buffer[256];
printf("Enter name: ");
gets(buffer);
printf("Hello, %s!\n", buffer);
```

**Warning:**  
No buffer overflow protection. Use `fgets()` instead for safer input.

---

### fgets()

```c
char *fgets(char *s, int size);
```

**Description:**  
Reads a line from keyboard with buffer size limit. Supports arrow keys for cursor movement.

**Parameters:**

- `s`: Buffer to store input
- `size`: Maximum characters to read (including null terminator)

**Returns:**  
Pointer to `s`, or NULL on error

**Features:**

- Buffer overflow protection
- Arrow key support (left/right)
- Insert mode editing
- Backspace support
- Visual editing feedback

**Example:**

```c
char buffer[256];
printf("Enter text: ");
fgets(buffer, sizeof(buffer));
printf("You entered: %s\n", buffer);
```

**Arrow Key Support:**

- **Left Arrow** - Move cursor left
- **Right Arrow** - Move cursor right
- **Backspace** - Delete character before cursor
- **Character** - Insert at cursor position

---

## Formatted Output

### printf()

```c
int printf(const char *format, ...);
```

**Description:**  
Prints formatted output to the screen.

**Parameters:**

- `format`: Format string with conversion specifiers
- `...`: Variable arguments matching format specifiers

**Returns:**  
Number of characters printed

**Format Specifiers:**

| Specifier   | Type         | Description                      |
| ----------- | ------------ | -------------------------------- |
| `%d` / `%i` | int          | Signed decimal integer           |
| `%u`        | unsigned int | Unsigned decimal integer         |
| `%x`        | unsigned int | Unsigned hexadecimal (lowercase) |
| `%X`        | unsigned int | Unsigned hexadecimal (uppercase) |
| `%o`        | unsigned int | Unsigned octal                   |
| `%s`        | char\*       | String                           |
| `%c`        | char         | Single character                 |
| `%p`        | void\*       | Pointer address (hex)            |
| `%%`        | -            | Literal '%' character            |

**Length Modifiers:**

- `l` - long (e.g., `%ld`)
- `ll` - long long (e.g., `%lld`)
- `h` - short (e.g., `%hd`)
- `hh` - char (e.g., `%hhd`)
- `z` - size_t (e.g., `%zu`)

**Flags:**

- `-` - Left-align
- `0` - Zero-pad numbers
- Width - Minimum field width (e.g., `%5d`)
- Precision - For strings/floats (e.g., `%.3s`)

**Examples:**

```c
printf("Number: %d\n", 42);
printf("Hex: 0x%x\n", 255);
printf("String: %s\n", "Hello");
printf("Pointer: %p\n", ptr);
printf("Padded: %05d\n", 42);        // "00042"
printf("Width: %10s\n", "Hi");       // "        Hi"
printf("Left: %-10s\n", "Hi");       // "Hi        "
printf("Precision: %.3s\n", "Hello"); // "Hel"
```

---

### sprintf()

```c
int sprintf(char *str, const char *format, ...);
```

**Description:**  
Formats output to a string buffer instead of screen.

**Parameters:**

- `str`: Destination buffer
- `format`: Format string
- `...`: Variable arguments

**Returns:**  
Number of characters written (excluding null terminator)

**Example:**

```c
char buffer[100];
sprintf(buffer, "Value: %d", 42);
// buffer now contains "Value: 42"
```

**Warning:**  
No buffer overflow protection. Use `snprintf()` instead.

---

### snprintf()

```c
int snprintf(char *str, size_t size, const char *format, ...);
```

**Description:**  
Formats output to a string buffer with size limit.

**Parameters:**

- `str`: Destination buffer
- `size`: Maximum bytes to write (including null terminator)
- `format`: Format string
- `...`: Variable arguments

**Returns:**  
Number of characters that would have been written (excluding null)

**Example:**

```c
char buffer[10];
int written = snprintf(buffer, sizeof(buffer), "Hello, World!");
// buffer contains "Hello, Wo" (truncated)
// written = 13 (full length)
```

**Notes:**

- Always null-terminates (if size > 0)
- Returns full length even if truncated
- Safe against buffer overflows

---

### vprintf()

```c
int vprintf(const char *format, va_list args);
```

**Description:**  
Variable argument version of `printf()`. Used internally by `printf()`.

**Parameters:**

- `format`: Format string
- `args`: Variable argument list

**Returns:**  
Number of characters printed

**Example:**

```c
void my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
    return result;
}
```

---

### vsprintf() / vsnprintf()

```c
int vsprintf(char *str, const char *format, va_list args);
int vsnprintf(char *str, size_t size, const char *format, va_list args);
```

**Description:**  
Variable argument versions of `sprintf()` and `snprintf()`.

**Example:**

```c
void log_message(const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    write_to_log(buffer);
}
```

---

## Terminal Control

### terminal_reset()

```c
void terminal_reset(void);
```

**Description:**  
Resets terminal cursor to top-left position (0, 0).

**Example:**

```c
terminal_reset();
puts("Starting fresh at top-left");
```

---

## Implementation Details

### VGA Integration

The stdio library interfaces directly with the VGA driver:

```
printf() → putchar() → vga_write_cell()
                    ↓
                vga_set_cursor_pos()
```

### Character Buffering

**Output:**

- No buffering - characters appear immediately
- Direct VGA buffer writes

**Input:**

- Keyboard driver provides 256-byte circular buffer
- `getchar()` reads from buffer
- Blocks (halts CPU) if buffer empty

### Format String Parsing

Printf implementation uses a state machine:

1. Parse flags (`-`, `0`)
2. Parse width (`%10d`)
3. Parse precision (`%.5s`)
4. Parse length modifier (`l`, `ll`)
5. Parse conversion specifier (`d`, `s`, etc.)
6. Format and output

### Memory Usage

- **No heap allocation** during I/O operations
- Small stack buffers for number conversion (64 bytes)
- `sprintf` family uses caller-provided buffers

### Thread Safety

**Not thread-safe!** The library assumes single-threaded execution:

- Global terminal position variables
- No locking around VGA writes
- Shared format buffers

For multi-threaded kernels, add spinlocks around I/O operations.

---

## Common Patterns

### Error Messages

```c
printf("[ERROR] Failed to allocate memory\n");
printf("[WARNING] Low memory: %lu bytes free\n", free_mem);
```

### Debugging

```c
#ifdef DEBUG
printf("[DEBUG] Function %s at line %d\n", __func__, __LINE__);
printf("[DEBUG] Variable value: %d\n", var);
#endif
```

### User Input

```c
char buffer[256];
printf("Enter command: ");
fgets(buffer, sizeof(buffer));

if (strcmp(buffer, "quit\n") == 0) {
    return;
}
```

### Formatting Tables

```c
printf("%-20s %10s %10s\n", "Name", "Size", "Used");
printf("%-20s %10lu %10lu\n", "Heap", heap_total, heap_used);
printf("%-20s %10lu %10lu\n", "Stack", stack_total, stack_used);
```

---

## Performance Considerations

### Fast Operations

- `putchar()` - Single VGA write
- `puts()` - Multiple putchar() calls
- Character I/O - Direct hardware access

### Slow Operations

- `printf()` with many specifiers - Complex parsing
- `getchar()` - Blocks waiting for input
- String formatting - Multiple conversions

### Optimization Tips

**Use puts() for simple strings:**

```c
// Slower:
printf("Hello\n");

// Faster:
puts("Hello");
```

**Avoid repeated formatting:**

```c
// Slower:
for (int i = 0; i < 1000; i++) {
    printf("Iteration %d\n", i);
}

// Faster:
char buf[32];
for (int i = 0; i < 1000; i++) {
    snprintf(buf, sizeof(buf), "Iteration %d\n", i);
    puts(buf);
}
```

---

## Limitations

1. **No floating-point support** - `%f`, `%e`, `%g` not implemented
2. **Limited precision** - Maximum 64-character number strings
3. **No buffering** - Every character written immediately
4. **No stream support** - Only console I/O (no files)
5. **Single-threaded** - No locking mechanisms
6. **ASCII only** - No UTF-8 or Unicode support

---

## See Also

- `STRING.md` - String manipulation functions

---

**Maintainer:** Trollycat  
**License:** MIT  
**Last Updated:** January 2026
