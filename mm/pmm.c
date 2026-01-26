/*
 * Copyright (c) 2026 Trollycat
 * Physical Memory Manager implementation
 * Uses bitmap-based allocation like Linux
 */

#include <thuban/pmm.h>
#include <thuban/stdio.h>
#include <thuban/string.h>

#define BITMAP_SIZE 32768 // supports up to 128MB with 4KB pages

static uint32_t bitmap[BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

extern uint64_t _kernel_end;

/*
 * Set's a bit in the bitmap
 */
static inline void bitmap_set(uint64_t bit)
{
    bitmap[bit / 32] |= (1 << (bit % 32));
}

/*
 * Clear's a bit in the bitmap
 */
static inline void bitmap_clear(uint64_t bit)
{
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

/*
 * Test's if a bit is set
 */
static inline int bitmap_test(uint64_t bit)
{
    return bitmap[bit / 32] & (1 << (bit % 32));
}

/*
 * Find's first free page
 */
static uint64_t find_free_page(void)
{
    for (uint64_t i = 0; i < total_pages; i++)
    {
        if (!bitmap_test(i))
        {
            return i;
        }
    }
    return (uint64_t)-1;
}

/*
 * Find's n contiguous free pages
 */
static uint64_t find_free_pages(size_t count)
{
    uint64_t found = 0;
    uint64_t start = 0;

    for (uint64_t i = 0; i < total_pages; i++)
    {
        if (!bitmap_test(i))
        {
            if (found == 0)
            {
                start = i;
            }
            found++;

            if (found == count)
            {
                return start;
            }
        }
        else
        {
            found = 0;
        }
    }

    return (uint64_t)-1;
}

/*
 * Initialize's the physical memory manager
 */
void pmm_init(uint64_t mem_size)
{
    total_pages = mem_size / PAGE_SIZE;

    if (total_pages > BITMAP_SIZE * 32)
    {
        total_pages = BITMAP_SIZE * 32;
    }

    // clear bitmap (all free)
    memset(bitmap, 0, sizeof(bitmap));
    used_pages = 0;

    // mark first 1MB + kernel as used
    uint64_t kernel_end_phys = (uint64_t)&_kernel_end - 0xFFFFFFFF80000000ULL;
    uint64_t kernel_pages = (kernel_end_phys + PAGE_SIZE - 1) / PAGE_SIZE;

    // reserve at least first MB plus kernel
    if (kernel_pages < 256) // minimum 1MB
    {
        kernel_pages = 256;
    }

    for (uint64_t i = 0; i < kernel_pages; i++)
    {
        bitmap_set(i);
        used_pages++;
    }
}

/*
 * Allocate's a single physical page
 */
void *pmm_alloc(void)
{
    uint64_t page = find_free_page();

    if (page == (uint64_t)-1)
    {
        return NULL;
    }

    bitmap_set(page);
    used_pages++;

    return (void *)(page * PAGE_SIZE);
}

/*
 * Allocate's multiple contiguous pages
 */
void *pmm_alloc_pages(size_t count)
{
    if (count == 0)
    {
        return NULL;
    }

    if (count == 1)
    {
        return pmm_alloc();
    }

    uint64_t start_page = find_free_pages(count);

    if (start_page == (uint64_t)-1)
    {
        return NULL;
    }

    for (size_t i = 0; i < count; i++)
    {
        bitmap_set(start_page + i);
        used_pages++;
    }

    return (void *)(start_page * PAGE_SIZE);
}

/*
 * Free's a physical page
 */
void pmm_free(void *page)
{
    if (!page)
    {
        return;
    }

    uint64_t page_num = (uint64_t)page / PAGE_SIZE;

    if (page_num >= total_pages)
    {
        return;
    }

    if (!bitmap_test(page_num))
    {
        return;
    }

    bitmap_clear(page_num);
    used_pages--;
}

/*
 * Free's multiple pages
 */
void pmm_free_pages(void *page, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        pmm_free((void *)((uint64_t)page + (i * PAGE_SIZE)));
    }
}

/*
 * Get's total memory in bytes
 */
uint64_t pmm_get_total_memory(void)
{
    return total_pages * PAGE_SIZE;
}

/*
 * Get's used memory in bytes
 */
uint64_t pmm_get_used_memory(void)
{
    return used_pages * PAGE_SIZE;
}

/*
 * Get's free memory in bytes
 */
uint64_t pmm_get_free_memory(void)
{
    return (total_pages - used_pages) * PAGE_SIZE;
}