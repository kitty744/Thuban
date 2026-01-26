/*
 * Copyright (c) 2026 Trollycat
 * String library for Thuban
 */

#include <thuban/string.h>

/*
 * Get's the length of a string
 * NOTE: This count's character's until it hit's the null terminator
 */
size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
    {
        len++;
    }
    return len;
}

/*
 * Get's the length of a string with a maximum limit
 */
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (len < maxlen && s[len])
    {
        len++;
    }
    return len;
}

/*
 * Copy's a string from src to dest
 * NOTE: Make sure dest has enough space
 */
char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

/*
 * Copy's n character's from src to dest
 */
char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }
    return dest;
}

/*
 * Concatenate's src to the end of dest
 */
char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d)
    {
        d++;
    }
    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

/*
 * Concatenate's n character's from src to dest
 */
char *strncat(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (*d)
    {
        d++;
    }
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
    {
        d[i] = src[i];
    }
    d[i] = '\0';
    return dest;
}

/*
 * Compare's two strings
 * RETURNS: 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/*
 * Compare's n character's of two strings
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }

    if (n == (size_t)-1)
        return 0;

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/*
 * Find's first occurrence of character in string
 */
char *strchr(const char *s, int c)
{
    while (*s != (char)c)
    {
        if (!*s)
            return NULL;
        s++;
    }
    return (char *)s;
}

/*
 * Find's last occurrence of character in string
 */
char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s)
    {
        if (*s == (char)c)
            last = s;
        s++;
    }
    if ((char)c == '\0')
        return (char *)s;
    return (char *)last;
}

/*
 * Find's substring needle in string haystack
 */
char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;

    for (; *haystack; haystack++)
    {
        const char *h = haystack;
        const char *n = needle;

        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }

        if (!*n)
            return (char *)haystack;
    }

    return NULL;
}

/*
 * Fill's memory with a constant byte
 */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
    {
        *p++ = (unsigned char)c;
    }
    return s;
}

/*
 * Copy's n bytes from src to dest
 * NOTE: Memory area's should not overlap
 */
void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    while (n--)
    {
        *d++ = *s++;
    }

    return dest;
}

/*
 * Copy's n bytes from src to dest
 * NOTE: Memory area's can overlap, this handle's it correctly
 */
void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s)
    {
        while (n--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        d += n;
        s += n;
        while (n--)
        {
            *--d = *--s;
        }
    }

    return dest;
}

/*
 * Compare's n bytes of memory
 * RETURNS: 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (n--)
    {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }

    return 0;
}