#include <thuban/stdio.h>
#include <thuban/vga.h>

static size_t term_x = 0;
static size_t term_y = 0;

static void term_scroll(void)
{
    vga_scroll_up();
    term_y = VGA_HEIGHT - 1;
}

static void term_newline(void)
{
    term_x = 0;
    term_y++;
    if (term_y >= VGA_HEIGHT)
    {
        term_scroll();
    }
    vga_set_cursor_pos(term_x, term_y);
}

static void term_advance(void)
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

int putc(int c)
{
    return putchar(c);
}

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
            vga_write_cell(' ', vga_get_color(), term_x, term_y);
            vga_set_cursor_pos(term_x, term_y);
        }
        return c;
    }

    vga_write_cell((char)c, vga_get_color(), term_x, term_y);
    term_advance();

    return c;
}

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

int getc(void)
{
    return -1;
}

int getchar(void)
{
    return getc();
}

char *gets(char *s)
{
    (void)s;
    return NULL;
}

char *fgets(char *s, int size)
{
    (void)s;
    (void)size;
    return NULL;
}

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

static void print_ptr(void *ptr)
{
    putchar('0');
    putchar('x');
    print_uint((unsigned long long)ptr, 16, 16, -1, 0, 1, 0);
}

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

int printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

int vsprintf(char *str, const char *format, va_list args)
{
    (void)str;
    (void)format;
    (void)args;
    return 0;
}

int sprintf(char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    return result;
}

int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    (void)str;
    (void)size;
    (void)format;
    (void)args;
    return 0;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}