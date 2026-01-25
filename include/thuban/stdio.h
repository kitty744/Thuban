#ifndef THUBAN_STDIO_H
#define THUBAN_STDIO_H

#include <stdarg.h>
#include <stddef.h>

// character I/O
int putc(int c);
int putchar(int c);
int puts(const char *s);
int getc(void);
int getchar(void);

// formatted output
int printf(const char *format, ...);
int vprintf(const char *format, va_list args);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vsprintf(char *str, const char *format, va_list args);
int vsnprintf(char *str, size_t size, const char *format, va_list args);

// string input
char *gets(char *s);
char *fgets(char *s, int size);

#endif