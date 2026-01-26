/*
 * Copyright (c) 2026 Trollycat
 * Dynamic heap allocator for Thuban
 */

#ifndef THUBAN_HEAP_H
#define THUBAN_HEAP_H

#include <stdint.h>
#include <stddef.h>

// initialize heap
void heap_init(void);

// allocate memory
void *malloc(size_t size);

// allocate and zero memory
void *calloc(size_t num, size_t size);

// reallocate memory
void *realloc(void *ptr, size_t size);

// free memory
void free(void *ptr);

// get heap statistics
uint64_t heap_get_total(void);
uint64_t heap_get_used(void);
uint64_t heap_get_free(void);

#endif