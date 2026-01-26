# Thuban Standard Library — string

## NAME

string — String and memory manipulation routines for the Thuban kernel

## SYNOPSIS

    #include <thuban/string.h>

## DESCRIPTION

The Thuban `string` library provides basic string handling and memory
manipulation routines. These functions are designed for kernel-space use
and provide functionality similar to the standard C library.

All functions operate on null-terminated strings unless explicitly stated
otherwise.

---

## STRING LENGTH FUNCTIONS

### strlen()

    size_t strlen(const char *s);

Returns the number of characters in the string `s` before the null terminator.

#### Example

    size_t len = strlen("kernel"); // 6

---

### strnlen()

    size_t strnlen(const char *s, size_t maxlen);

Returns the number of characters in `s`, but at most `maxlen`.

---

## STRING COPY FUNCTIONS

### strcpy()

    char *strcpy(char *dest, const char *src);

Copies the string `src` (including null terminator) into `dest`.
Make sure `dest` has enough space.

---

### strncpy()

    char *strncpy(char *dest, const char *src, size_t n);

Copies at most `n` characters from `src` to `dest`. Pads with `\0`
if `src` is shorter than `n`.

---

## STRING CONCATENATION FUNCTIONS

### strcat()

    char *strcat(char *dest, const char *src);

Appends `src` to the end of `dest`. `dest` must have enough space.

---

### strncat()

    char *strncat(char *dest, const char *src, size_t n);

Appends at most `n` characters from `src` to `dest`. Adds a null
terminator.

---

## STRING COMPARISON FUNCTIONS

### strcmp()

    int strcmp(const char *s1, const char *s2);

Compares two strings.

- Returns `0` if `s1` and `s2` are equal
- Returns negative if `s1 < s2`
- Returns positive if `s1 > s2`

---

### strncmp()

    int strncmp(const char *s1, const char *s2, size_t n);

Compares at most `n` characters of two strings.

---

## STRING SEARCH FUNCTIONS

### strchr()

    char *strchr(const char *s, int c);

Returns a pointer to the first occurrence of character `c` in string `s`,
or `NULL` if not found.

---

### strrchr()

    char *strrchr(const char *s, int c);

Returns a pointer to the last occurrence of character `c` in string `s`,
or `NULL` if not found.

---

### strstr()

    char *strstr(const char *haystack, const char *needle);

Returns a pointer to the first occurrence of substring `needle` in
`haystack`. Returns `haystack` if `needle` is empty, or `NULL` if not found.

---

## MEMORY FUNCTIONS

### memset()

    void *memset(void *s, int c, size_t n);

Fills the memory area `s` with `n` bytes of value `c`.

---

### memcpy()

    void *memcpy(void *dest, const void *src, size_t n);

Copies `n` bytes from `src` to `dest`. Memory areas **must not overlap**.

---

### memmove()

    void *memmove(void *dest, const void *src, size_t n);

Copies `n` bytes from `src` to `dest`. Correctly handles overlapping memory.

---

### memcmp()

    int memcmp(const void *s1, const void *s2, size_t n);

Compares `n` bytes of memory.

- Returns `0` if equal
- Returns negative if `s1 < s2`
- Returns positive if `s1 > s2`

---

## NOTES

- Functions assume ASCII character encoding
- No dynamic memory allocation is performed
- Designed for kernel-space use
- `strcpy`, `strcat`, and similar functions do not perform bounds checking
- `memcpy` does not handle overlapping memory (use `memmove` in that case)

