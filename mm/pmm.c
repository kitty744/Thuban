/*
 * Copyright (c) 2026 Trollycat
 * Physical Memory Manager implementation
 * Uses bitmap-based allocation
 */

#include <thuban/pmm.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/spinlock.h>

#define BITMAP_SIZE 32768 // supports up to 128MB with 4KB pages

static uint32_t bitmap[BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

extern uint64_t _kernel_end;

/* Spinlock to protect PMM operations */
static spinlock_t pmm_lock = SPINLOCK_INIT_NAMED("pmm");

/*
 * Set's a bit in the bitmap
 * NOTE: Must be called with pmm_lock held
 */
static inline void bitmap_set(uint64_t bit)
{
    bitmap[bit / 32] |= (1 << (bit % 32));
}

/*
 * Clear's a bit in the bitmap
 * NOTE: Must be called with pmm_lock held
 */
static inline void bitmap_clear(uint64_t bit)
{
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

/*
 * Test's if a bit is set
 * NOTE: Must be called with pmm_lock held
 */
static inline int bitmap_test(uint64_t bit)
{
    return bitmap[bit / 32] & (1 << (bit % 32));
}

/*
 * Find's first free page
 * NOTE: Must be called with pmm_lock held
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
 * NOTE: Must be called with pmm_lock held
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

    spin_lock_init(&pmm_lock, "pmm");
}

/*
 * Allocate's a single physical page
 */
void *pmm_alloc(void)
{
    spin_lock(&pmm_lock);

    uint64_t page = find_free_page();

    if (page == (uint64_t)-1)
    {
        spin_unlock(&pmm_lock);
        return NULL;
    }

    bitmap_set(page);
    used_pages++;

    void *addr = (void *)(page * PAGE_SIZE);
    spin_unlock(&pmm_lock);
    return addr;
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

    spin_lock(&pmm_lock);

    uint64_t start_page = find_free_pages(count);

    if (start_page == (uint64_t)-1)
    {
        spin_unlock(&pmm_lock);
        return NULL;
    }

    for (size_t i = 0; i < count; i++)
    {
        bitmap_set(start_page + i);
        used_pages++;
    }

    void *addr = (void *)(start_page * PAGE_SIZE);
    spin_unlock(&pmm_lock);
    return addr;
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

    spin_lock(&pmm_lock);

    uint64_t page_num = (uint64_t)page / PAGE_SIZE;

    if (page_num >= total_pages)
    {
        spin_unlock(&pmm_lock);
        return;
    }

    if (!bitmap_test(page_num))
    {
        spin_unlock(&pmm_lock);
        return;
    }

    bitmap_clear(page_num);
    used_pages--;

    spin_unlock(&pmm_lock);
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
    spin_lock(&pmm_lock);
    uint64_t total = total_pages * PAGE_SIZE;
    spin_unlock(&pmm_lock);
    return total;
}

/*
 * Get's used memory in bytes
 */
uint64_t pmm_get_used_memory(void)
{
    spin_lock(&pmm_lock);
    uint64_t used = used_pages * PAGE_SIZE;
    spin_unlock(&pmm_lock);
    return used;
}

/*
 * Get's free memory in bytes
 */
uint64_t pmm_get_free_memory(void)
{
    spin_lock(&pmm_lock);
    uint64_t free = (total_pages - used_pages) * PAGE_SIZE;
    spin_unlock(&pmm_lock);
    return free;
}