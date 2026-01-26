/*
 * Copyright (c) 2026 Trollycat
 * Physical Memory Manager for Thuban
 */

#ifndef THUBAN_PMM_H
#define THUBAN_PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

// initialize physical memory manager
void pmm_init(uint64_t mem_size);

// allocate physical page
void *pmm_alloc(void);

// allocate multiple contiguous pages
void *pmm_alloc_pages(size_t count);

// free physical page
void pmm_free(void *page);

// free multiple pages
void pmm_free_pages(void *page, size_t count);

// get memory statistics
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_free_memory(void);

#endif