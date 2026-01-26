/*
 * Copyright (c) 2026 Trollycat
 * Standard library for Thuban
 */

#include <thuban/stdio.h>
#include <thuban/vga.h>
#include <thuban/string.h>
#include <thuban/keyboard.h>

static size_t term_x = 0;
static size_t term_y = 0;

/*
 * Reset's terminal tracking to top-left
 */
void terminal_reset(void)
{
    term_x = 0;
    term_y = 0;
}

/*
 * Scroll's the terminal up one line
 */
void term_scroll(void)
{
    vga_scroll_up();
    term_y = VGA_HEIGHT - 1;
}

/*
 * Create's a newline on the terminal screen
 */
void term_newline(void)
{
    term_x = 0;
    term_y++;
    if (term_y >= VGA_HEIGHT)
    {
        term_scroll();
    }
    vga_set_cursor_pos(term_x, term_y);
}

/*
 * Advance's the cursor and wrap's text to next line if needed
 */
void term_advance(void)
{
    term_x++;
    if (term_x >= VGA_WIDTH)
    {
        term_newline();
    }
    else
    {
        vga_set_cursor_pos(term_x, term_y);
    }
}

/*
 * Simple wrapper for putchar
 * NOTE: Actually call's putchar, putc is just a wrapper for a simpler name
 */
int putc(int c)
{
    return putchar(c);
}

/*
 * Put's a character onto the screen using VGA driver
 */
int putchar(int c)
{
    if (c == '\n')
    {
        term_newline();
        return c;
    }

    if (c == '\r')
    {
        term_x = 0;
        vga_set_cursor_pos(term_x, term_y);
        return c;
    }

    if (c == '\t')
    {
        size_t spaces = 4 - (term_x % 4);
        for (size_t i = 0; i < spaces; i++)
        {
            putchar(' ');
        }
        return c;
    }

    if (c == '\b')
    {
        if (term_x > 0)
        {
            term_x--;
            vga_set_cursor_pos(term_x, term_y);
        }
        return c;
    }

    vga_write_cell((char)c, vga_get_color(), term_x, term_y);
    term_advance();

    return c;
}

/*
 * Put's text onto the screen using VGA driver
 * NOTE: This actually just take's the string input and call's putchar for every character in the string
 * EXAMPLE: puts("Hello, kernel!")
 */
int puts(const char *s)
{
    if (!s)
        return -1;

    int count = 0;
    while (*s)
    {
        putchar(*s++);
        count++;
    }
    putchar('\n');
    return count + 1;
}

/*
 * Get's a character from input
 * NOTE: Now implemented using keyboard driver
 */
int getchar(void)
{
    while (!keyboard_available())
    {
        asm volatile("hlt");
    }
    return keyboard_getchar();
}

/*
 * Wrapper for getchar
 */
int getc(void)
{
    return getchar();
}

/*
 * Get's a string from input
 * NOTE: Reads until newline and null-terminates
 */
char *gets(char *s)
{
    if (!s)
        return NULL;

    int i = 0;
    while (1)
    {
        int c = getchar();

        if (c == '\n' || c == '\r')
        {
            s[i] = '\0';
            putchar('\n');
            break;
        }
        else if (c == '\b')
        {
            if (i > 0)
            {
                i--;
                putchar('\b');
            }
        }
        else if (c >= 32 && c <= 126)
        {
            s[i++] = c;
            putchar(c);
        }
    }

    return s;
}

/*
 * Get's a string from input with size limit
 * NOTE: Reads at most size-1 characters with arrow key support
 */
char *fgets(char *s, int size)
{
    if (!s || size <= 0)
        return NULL;

    int len = 0;
    int cursor_pos = 0;

    while (1)
    {
        int c = getchar();

        // check for escape sequences (arrow keys)
        if (c == 0x1B)
        {
            int c2 = getchar();
            if (c2 == '[')
            {
                int c3 = getchar();
                if (c3 == 'D') // left arrow
                {
                    if (cursor_pos > 0)
                    {
                        cursor_pos--;
                        putchar('\b');
                    }
                }
                else if (c3 == 'C') // right arrow
                {
                    if (cursor_pos < len)
                    {
                        putchar(s[cursor_pos]);
                        cursor_pos++;
                    }
                }
            }
            continue;
        }

        if (c == '\n' || c == '\r')
        {
            s[len] = '\0';
            putchar('\n');
            break;
        }
        else if (c == '\b')
        {
            if (cursor_pos > 0)
            {
                // delete character before cursor
                for (int i = cursor_pos - 1; i < len - 1; i++)
                {
                    s[i] = s[i + 1];
                }
                len--;
                cursor_pos--;

                // redraw line
                putchar('\b');
                for (int i = cursor_pos; i < len; i++)
                {
                    putchar(s[i]);
                }
                putchar(' ');

                // move cursor back
                for (int i = 0; i <= len - cursor_pos; i++)
                {
                    putchar('\b');
                }
            }
        }
        else if (c >= 32 && c <= 126 && len < size - 1)
        {
            // insert character at cursor position
            for (int i = len; i > cursor_pos; i--)
            {
                s[i] = s[i - 1];
            }
            s[cursor_pos] = c;
            len++;

            // redraw from cursor
            for (int i = cursor_pos; i < len; i++)
            {
                putchar(s[i]);
            }
            cursor_pos++;

            // move cursor back
            for (int i = cursor_pos; i < len; i++)
            {
                putchar('\b');
            }
        }
    }

    return s;
}

/*
 * Print's a string to the screen with formatting option's
 * NOTE: This is for precise formatting
 * You will likely use puts("") or printf("") instead
 */
static void print_string(const char *s, int width, int precision, int left_align)
{
    int len = 0;
    const char *p = s;
    while (*p && (precision < 0 || len < precision))
    {
        len++;
        p++;
    }

    int padding = width - len;
    if (padding < 0)
        padding = 0;

    if (!left_align)
    {
        for (int i = 0; i < padding; i++)
            putchar(' ');
    }

    for (int i = 0; i < len; i++)
    {
        putchar(s[i]);
    }

    if (left_align)
    {
        for (int i = 0; i < padding; i++)
            putchar(' ');
    }
}

/*
 * Precise way to print integer to the screen
 */
static void print_int(long long num, int base, int width, int precision, int left_align, int zero_pad, int sign, int uppercase)
{
    char buffer[64];
    int i = 0;
    int negative = 0;

    if (num == 0)
    {
        buffer[i++] = '0';
    }
    else
    {
        if (sign && num < 0)
        {
            negative = 1;
            num = -num;
        }

        unsigned long long n = (unsigned long long)num;
        while (n > 0)
        {
            int digit = n % base;
            if (digit < 10)
            {
                buffer[i++] = '0' + digit;
            }
            else
            {
                buffer[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
            }
            n /= base;
        }
    }

    while (i < precision)
    {
        buffer[i++] = '0';
    }

    if (negative)
    {
        buffer[i++] = '-';
    }

    int len = i;
    int padding = width - len;
    if (padding < 0)
        padding = 0;

    if (!left_align && !zero_pad)
    {
        for (int j = 0; j < padding; j++)
            putchar(' ');
    }

    if (negative && zero_pad)
    {
        putchar('-');
        i--;
    }

    if (!left_align && zero_pad)
    {
        for (int j = 0; j < padding; j++)
            putchar('0');
    }

    while (i > 0)
    {
        putchar(buffer[--i]);
    }

    if (left_align)
    {
        for (int j = 0; j < padding; j++)
            putchar(' ');
    }
}

/*
 * Precise way to print unsigned integer to the screen
 */
static void print_uint(unsigned long long num, int base, int width, int precision, int left_align, int zero_pad, int uppercase)
{
    char buffer[64];
    int i = 0;

    if (num == 0)
    {
        buffer[i++] = '0';
    }
    else
    {
        while (num > 0)
        {
            int digit = num % base;
            if (digit < 10)
            {
                buffer[i++] = '0' + digit;
            }
            else
            {
                buffer[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
            }
            num /= base;
        }
    }

    while (i < precision)
    {
        buffer[i++] = '0';
    }

    int len = i;
    int padding = width - len;
    if (padding < 0)
        padding = 0;

    if (!left_align && !zero_pad)
    {
        for (int j = 0; j < padding; j++)
            putchar(' ');
    }

    if (!left_align && zero_pad)
    {
        for (int j = 0; j < padding; j++)
            putchar('0');
    }

    while (i > 0)
    {
        putchar(buffer[--i]);
    }

    if (left_align)
    {
        for (int j = 0; j < padding; j++)
            putchar(' ');
    }
}

/*
 * Print's a pointer to the screen
 */
static void print_ptr(void *ptr)
{
    putchar('0');
    putchar('x');
    print_uint((unsigned long long)ptr, 16, 16, -1, 0, 1, 0);
}

/*
 * Handle's formatting for printf
 * NOTE: This method is called by printf and will likely not be called by the programmer directly
 * This is just to separate formatting logic from printf
 */
int vprintf(const char *format, va_list args)
{
    int count = 0;

    while (*format)
    {
        if (*format != '%')
        {
            putchar(*format++);
            count++;
            continue;
        }

        format++;

        int left_align = 0;
        int zero_pad = 0;
        int width = 0;
        int precision = -1;
        int long_flag = 0;
        int long_long_flag = 0;

        // flags
        while (*format == '-' || *format == '0')
        {
            if (*format == '-')
                left_align = 1;
            if (*format == '0')
                zero_pad = 1;
            format++;
        }

        // width
        if (*format >= '0' && *format <= '9')
        {
            while (*format >= '0' && *format <= '9')
            {
                width = width * 10 + (*format - '0');
                format++;
            }
        }
        else if (*format == '*')
        {
            width = va_arg(args, int);
            format++;
        }

        // precision
        if (*format == '.')
        {
            format++;
            precision = 0;
            if (*format >= '0' && *format <= '9')
            {
                while (*format >= '0' && *format <= '9')
                {
                    precision = precision * 10 + (*format - '0');
                    format++;
                }
            }
            else if (*format == '*')
            {
                precision = va_arg(args, int);
                format++;
            }
        }

        // length modifiers
        if (*format == 'l')
        {
            format++;
            long_flag = 1;
            if (*format == 'l')
            {
                format++;
                long_long_flag = 1;
            }
        }
        else if (*format == 'h')
        {
            format++;
            if (*format == 'h')
                format++;
        }
        else if (*format == 'z' || *format == 't')
        {
            format++;
            long_flag = 1;
        }

        // conversion specifiers
        switch (*format)
        {
        case 'c':
        {
            char c = (char)va_arg(args, int);
            putchar(c);
            count++;
            break;
        }
        case 's':
        {
            const char *s = va_arg(args, const char *);
            if (!s)
                s = "(null)";
            print_string(s, width, precision, left_align);
            count += strlen(s);
            break;
        }
        case 'd':
        case 'i':
        {
            long long num;
            if (long_long_flag)
                num = va_arg(args, long long);
            else if (long_flag)
                num = va_arg(args, long);
            else
                num = va_arg(args, int);
            print_int(num, 10, width, precision, left_align, zero_pad, 1, 0);
            count++;
            break;
        }
        case 'u':
        {
            unsigned long long num;
            if (long_long_flag)
                num = va_arg(args, unsigned long long);
            else if (long_flag)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);
            print_uint(num, 10, width, precision, left_align, zero_pad, 0);
            count++;
            break;
        }
        case 'x':
        {
            unsigned long long num;
            if (long_long_flag)
                num = va_arg(args, unsigned long long);
            else if (long_flag)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);
            print_uint(num, 16, width, precision, left_align, zero_pad, 0);
            count++;
            break;
        }
        case 'X':
        {
            unsigned long long num;
            if (long_long_flag)
                num = va_arg(args, unsigned long long);
            else if (long_flag)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);
            print_uint(num, 16, width, precision, left_align, zero_pad, 1);
            count++;
            break;
        }
        case 'o':
        {
            unsigned long long num;
            if (long_long_flag)
                num = va_arg(args, unsigned long long);
            else if (long_flag)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);
            print_uint(num, 8, width, precision, left_align, zero_pad, 0);
            count++;
            break;
        }
        case 'p':
        {
            void *ptr = va_arg(args, void *);
            print_ptr(ptr);
            count++;
            break;
        }
        case '%':
        {
            putchar('%');
            count++;
            break;
        }
        default:
            putchar(*format);
            count++;
            break;
        }

        format++;
    }

    return count;
}

/*
 * Print's thing's to the screen with formatting
 * You could just use printf("Hello Kernel") however in that case you should just simply do puts("Hello Kernel")
 * This method is for formatting
 * EXAMPLE: printf("Hello, Kernel! This is an integer formatting example: %d", 1)
 * Formatting start's with '%' then the type
 * There is many formatting option's
 * Most common one's are:
 *  - %d (integer)
 *  - %s (string)
 *  - %c (char)
 * NOTE: Actual formatting is handled in vprintf this method call's vprintf
 */
int printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

// buffer state for sprintf/snprintf
static char *sprintf_buffer = NULL;
static size_t sprintf_buffer_size = 0;
static size_t sprintf_buffer_pos = 0;

/*
 * Internal putchar for sprintf that writes to buffer
 */
static int sprintf_putchar(int c)
{
    if (sprintf_buffer && sprintf_buffer_pos < sprintf_buffer_size - 1)
    {
        sprintf_buffer[sprintf_buffer_pos++] = c;
        return c;
    }
    return -1;
}

/*
 * Internal vprintf that uses custom putchar
 */
static int vprintf_internal(const char *format, va_list args, int (*putchar_func)(int))
{
    int count = 0;

    while (*format)
    {
        if (*format != '%')
        {
            putchar_func(*format++);
            count++;
            continue;
        }

        format++;

        int left_align = 0;
        int zero_pad = 0;
        int width = 0;
        int precision = -1;
        int long_flag = 0;
        int long_long_flag = 0;

        // flags
        while (*format == '-' || *format == '0')
        {
            if (*format == '-')
                left_align = 1;
            if (*format == '0')
                zero_pad = 1;
            format++;
        }

        // width
        if (*format >= '0' && *format <= '9')
        {
            while (*format >= '0' && *format <= '9')
            {
                width = width * 10 + (*format - '0');
                format++;
            }
        }
        else if (*format == '*')
        {
            width = va_arg(args, int);
            format++;
        }

        // precision
        if (*format == '.')
        {
            format++;
            precision = 0;
            if (*format >= '0' && *format <= '9')
            {
                while (*format >= '0' && *format <= '9')
                {
                    precision = precision * 10 + (*format - '0');
                    format++;
                }
            }
            else if (*format == '*')
            {
                precision = va_arg(args, int);
                format++;
            }
        }

        // length modifiers
        if (*format == 'l')
        {
            format++;
            long_flag = 1;
            if (*format == 'l')
            {
                format++;
                long_long_flag = 1;
            }
        }
        else if (*format == 'h')
        {
            format++;
            if (*format == 'h')
                format++;
        }
        else if (*format == 'z' || *format == 't')
        {
            format++;
            long_flag = 1;
        }

        // conversion specifiers
        switch (*format)
        {
        case 'c':
        {
            char c = (char)va_arg(args, int);
            putchar_func(c);
            count++;
            break;
        }
        case 's':
        {
            const char *s = va_arg(args, const char *);
            if (!s)
                s = "(null)";
            int len = 0;
            while (s[len] && (precision < 0 || len < precision))
            {
                putchar_func(s[len]);
                len++;
            }
            count += len;
            break;
        }
        case 'd':
        case 'i':
        {
            long long num;
            if (long_long_flag)
                num = va_arg(args, long long);
            else if (long_flag)
                num = va_arg(args, long);
            else
                num = va_arg(args, int);

            char buffer[64];
            int i = 0;
            int negative = 0;

            if (num == 0)
            {
                buffer[i++] = '0';
            }
            else
            {
                if (num < 0)
                {
                    negative = 1;
                    num = -num;
                }

                unsigned long long n = (unsigned long long)num;
                while (n > 0)
                {
                    buffer[i++] = '0' + (n % 10);
                    n /= 10;
                }
            }

            if (negative)
            {
                buffer[i++] = '-';
            }

            while (i > 0)
            {
                putchar_func(buffer[--i]);
                count++;
            }
            break;
        }
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        {
            unsigned long long num;
            if (long_long_flag)
                num = va_arg(args, unsigned long long);
            else if (long_flag)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);

            int base = (*format == 'o') ? 8 : ((*format == 'x' || *format == 'X') ? 16 : 10);
            int uppercase = (*format == 'X');

            char buffer[64];
            int i = 0;

            if (num == 0)
            {
                buffer[i++] = '0';
            }
            else
            {
                while (num > 0)
                {
                    int digit = num % base;
                    if (digit < 10)
                    {
                        buffer[i++] = '0' + digit;
                    }
                    else
                    {
                        buffer[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
                    }
                    num /= base;
                }
            }

            while (i > 0)
            {
                putchar_func(buffer[--i]);
                count++;
            }
            break;
        }
        case 'p':
        {
            void *ptr = va_arg(args, void *);
            putchar_func('0');
            putchar_func('x');
            count += 2;

            unsigned long long num = (unsigned long long)ptr;
            char buffer[64];
            int i = 0;

            for (int j = 0; j < 16; j++)
            {
                int digit = (num >> (60 - j * 4)) & 0xF;
                buffer[i++] = digit < 10 ? '0' + digit : 'a' + (digit - 10);
            }

            for (int j = 0; j < i; j++)
            {
                putchar_func(buffer[j]);
                count++;
            }
            break;
        }
        case '%':
        {
            putchar_func('%');
            count++;
            break;
        }
        default:
            putchar_func(*format);
            count++;
            break;
        }

        format++;
    }

    return count;
}

/*
 * Print's formatted output to a string buffer
 */
int vsprintf(char *str, const char *format, va_list args)
{
    sprintf_buffer = str;
    sprintf_buffer_size = (size_t)-1;
    sprintf_buffer_pos = 0;

    int result = vprintf_internal(format, args, sprintf_putchar);

    if (str)
    {
        str[sprintf_buffer_pos] = '\0';
    }

    return result;
}

/*
 * Print's formatted output to a string buffer
 */
int sprintf(char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    return result;
}

/*
 * Print's formatted output to a string buffer with size limit
 */
int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    if (size == 0)
    {
        return 0;
    }

    sprintf_buffer = str;
    sprintf_buffer_size = size;
    sprintf_buffer_pos = 0;

    int result = vprintf_internal(format, args, sprintf_putchar);

    if (str && size > 0)
    {
        str[sprintf_buffer_pos < size ? sprintf_buffer_pos : size - 1] = '\0';
    }

    return result;
}

/*
 * Print's formatted output to a string buffer with size limit
 */
int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}