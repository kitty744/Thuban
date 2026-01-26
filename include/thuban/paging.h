/*
 * Copyright (c) 2026 Trollycat
 * Paging utilities for Thuban
 */

#ifndef THUBAN_PAGING_H
#define THUBAN_PAGING_H

#include <stdint.h>

// page table entry flags
#define PAGING_PRESENT 0x01
#define PAGING_WRITE 0x02
#define PAGING_USER 0x04
#define PAGING_WRITETHROUGH 0x08
#define PAGING_CACHE_DISABLE 0x10
#define PAGING_ACCESSED 0x20
#define PAGING_DIRTY 0x40
#define PAGING_HUGE 0x80
#define PAGING_GLOBAL 0x100
#define PAGING_NX (1ULL << 63)

// initialize paging
void paging_init(void);

// map a page
void paging_map(uint64_t virt, uint64_t phys, uint64_t flags);

// unmap a page
void paging_unmap(uint64_t virt);

// get physical address from virtual
uint64_t paging_get_phys(uint64_t virt);

// invalidate TLB for address
void paging_invalidate(uint64_t virt);

#endif