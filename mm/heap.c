/*
 * Copyright (c) 2026 Trollycat
 * Dynamic heap implementation
 * Starts with static heap then expands dynamically using VMM
 */

#include <thuban/heap.h>
#include <thuban/vmm.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/spinlock.h>

#define HEAP_MAGIC 0x48454150          // "HEAP"
#define INITIAL_HEAP_SIZE (256 * 1024) // 256KB initial

typedef struct heap_block
{
    uint32_t magic;
    size_t size;
    int free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static uint8_t initial_heap[INITIAL_HEAP_SIZE] __attribute__((aligned(16)));
static heap_block_t *heap_start = NULL;
static size_t total_heap_size = 0;
static size_t used_heap_size = 0;

/* Spinlock to protect heap operations */
static spinlock_t heap_lock = SPINLOCK_INIT_NAMED("heap");

/*
 * Initialize's the heap
 */
void heap_init(void)
{
    heap_start = (heap_block_t *)initial_heap;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = INITIAL_HEAP_SIZE - sizeof(heap_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;

    total_heap_size = INITIAL_HEAP_SIZE;
    used_heap_size = sizeof(heap_block_t);

    spin_lock_init(&heap_lock, "heap");
}

/*
 * Expand's the heap by allocating more pages
 * NOTE: Must be called with heap_lock held
 */
static int expand_heap(size_t needed_size)
{
    size_t pages = (needed_size + sizeof(heap_block_t) + 4095) / 4096;

    void *new_mem = vmm_alloc(pages, PAGE_WRITE);
    if (!new_mem)
    {
        return 0;
    }

    heap_block_t *new_block = (heap_block_t *)new_mem;
    new_block->magic = HEAP_MAGIC;
    new_block->size = (pages * 4096) - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = NULL;
    new_block->prev = NULL;

    // add to end of heap list
    heap_block_t *current = heap_start;
    while (current->next)
    {
        current = current->next;
    }

    current->next = new_block;
    new_block->prev = current;

    total_heap_size += pages * 4096;

    return 1;
}

/*
 * Coalesce's adjacent free blocks
 * NOTE: Must be called with heap_lock held
 */
static void coalesce(heap_block_t *block)
{
    if (block->next && block->next->free)
    {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next)
        {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->free)
    {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next)
        {
            block->next->prev = block->prev;
        }
    }
}

/*
 * Allocate's memory from heap
 */
void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size = (size + 15) & ~15; // align to 16 bytes

    spin_lock(&heap_lock);

    heap_block_t *current = heap_start;

    while (current)
    {
        if (current->magic != HEAP_MAGIC)
        {
            spin_unlock(&heap_lock);
            printf("[HEAP] Corruption detected at 0x%llx\n", (uint64_t)current);
            return NULL;
        }

        if (current->free && current->size >= size)
        {
            // split block if there's enough space
            if (current->size >= size + sizeof(heap_block_t) + 64)
            {
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)current + sizeof(heap_block_t) + size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = 1;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next)
                {
                    current->next->prev = new_block;
                }

                current->next = new_block;
                current->size = size;
            }

            current->free = 0;
            used_heap_size += current->size + sizeof(heap_block_t);

            void *ptr = (void *)((uint8_t *)current + sizeof(heap_block_t));
            spin_unlock(&heap_lock);
            return ptr;
        }

        current = current->next;
    }

    // no suitable block found expand heap
    if (!expand_heap(size))
    {
        spin_unlock(&heap_lock);
        return NULL;
    }

    spin_unlock(&heap_lock);
    return malloc(size);
}

/*
 * Allocate's and zero's memory
 */
void *calloc(size_t num, size_t size)
{
    size_t total = num * size;
    void *ptr = malloc(total);

    if (ptr)
    {
        memset(ptr, 0, total);
    }

    return ptr;
}

/*
 * Reallocate's memory
 */
void *realloc(void *ptr, size_t size)
{
    if (!ptr)
    {
        return malloc(size);
    }

    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    spin_lock(&heap_lock);

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    if (block->magic != HEAP_MAGIC)
    {
        spin_unlock(&heap_lock);
        return NULL;
    }

    if (block->size >= size)
    {
        spin_unlock(&heap_lock);
        return ptr;
    }

    spin_unlock(&heap_lock);

    void *new_ptr = malloc(size);
    if (!new_ptr)
    {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    free(ptr);

    return new_ptr;
}

/*
 * Free's allocated memory
 */
void free(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    spin_lock(&heap_lock);

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    if (block->magic != HEAP_MAGIC)
    {
        spin_unlock(&heap_lock);
        printf("[HEAP] Invalid free at 0x%llx\n", (uint64_t)ptr);
        return;
    }

    if (block->free)
    {
        spin_unlock(&heap_lock);
        printf("[HEAP] Double free detected at 0x%llx\n", (uint64_t)ptr);
        return;
    }

    block->free = 1;
    used_heap_size -= block->size + sizeof(heap_block_t);

    coalesce(block);

    spin_unlock(&heap_lock);
}

/*
 * Get's total heap size
 */
uint64_t heap_get_total(void)
{
    spin_lock(&heap_lock);
    uint64_t total = total_heap_size;
    spin_unlock(&heap_lock);
    return total;
}

/*
 * Get's used heap size
 */
uint64_t heap_get_used(void)
{
    spin_lock(&heap_lock);
    uint64_t used = used_heap_size;
    spin_unlock(&heap_lock);
    return used;
}

/*
 * Get's free heap size
 */
uint64_t heap_get_free(void)
{
    spin_lock(&heap_lock);
    uint64_t free = total_heap_size - used_heap_size;
    spin_unlock(&heap_lock);
    return free;
}