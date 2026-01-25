// simple VGA text mode driver
static unsigned short *vga = (unsigned short *)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;

void clear_screen(void)
{
    for (int i = 0; i < 80 * 25; i++)
    {
        vga[i] = 0x0F00 | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void putchar(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 25)
        {
            cursor_y = 24;
        }
        return;
    }

    int pos = cursor_y * 80 + cursor_x;
    vga[pos] = 0x0F00 | c;

    cursor_x++;
    if (cursor_x >= 80)
    {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 25)
        {
            cursor_y = 24;
        }
    }
}

void print(const char *str)
{
    while (*str)
    {
        putchar(*str);
        str++;
    }
}

void kmain(void)
{
    clear_screen();
    print("Thuban OS\n");
    print("Boot successful\n");

    while (1)
    {
        asm volatile("hlt");
    }
}