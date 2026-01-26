/*
 * Copyright (c) 2026 Trollycat
 * Virtual Memory Manager for Thuban
 */

#ifndef THUBAN_VMM_H
#define THUBAN_VMM_H

#include <stdint.h>
#include <stddef.h>

// page flags
#define PAGE_PRESENT 0x01
#define PAGE_WRITE 0x02
#define PAGE_USER 0x04

// initialize virtual memory manager
void vmm_init(void);

// map virtual to physical
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);

// unmap virtual address
void vmm_unmap(uint64_t virt);

// allocate virtual pages
void *vmm_alloc(size_t pages, uint64_t flags);

// free virtual pages
void vmm_free(void *virt, size_t pages);

// get physical address from virtual
uint64_t vmm_get_phys(uint64_t virt);

#endif