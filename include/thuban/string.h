/*
 * Copyright (c) 2026 Trollycat
 * String library for Thuban
 */

#ifndef THUBAN_STRING_H
#define THUBAN_STRING_H

#include <stddef.h>

// string length
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);

// string copy
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

// string concatenation
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);

// string comparison
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

// string search
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

// memory operations
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif