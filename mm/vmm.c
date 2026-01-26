/*
 * Copyright (c) 2026 Trollycat
 * Virtual Memory Manager implementation
 */

#include <thuban/vmm.h>
#include <thuban/pmm.h>
#include <thuban/stdio.h>
#include <thuban/string.h>

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000ULL
#define PAGE_SIZE 4096

extern uint64_t p4_table;

static uint64_t next_virt_addr = 0xFFFFFFFFC0000000ULL;

/*
 * Get's page table entry
 */
static uint64_t *get_pte(uint64_t virt, int create)
{
    uint64_t *pml4 = &p4_table;

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PAGE_PRESENT))
    {
        if (!create)
            return NULL;

        void *page = pmm_alloc();
        if (!page)
            return NULL;

        memset((void *)((uint64_t)page + KERNEL_VIRT_BASE), 0, PAGE_SIZE);
        pml4[pml4_idx] = (uint64_t)page | PAGE_PRESENT | PAGE_WRITE;
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT))
    {
        if (!create)
            return NULL;

        void *page = pmm_alloc();
        if (!page)
            return NULL;

        memset((void *)((uint64_t)page + KERNEL_VIRT_BASE), 0, PAGE_SIZE);
        pdpt[pdpt_idx] = (uint64_t)page | PAGE_PRESENT | PAGE_WRITE;
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pd[pd_idx] & PAGE_PRESENT))
    {
        if (!create)
            return NULL;

        void *page = pmm_alloc();
        if (!page)
            return NULL;

        memset((void *)((uint64_t)page + KERNEL_VIRT_BASE), 0, PAGE_SIZE);
        pd[pd_idx] = (uint64_t)page | PAGE_PRESENT | PAGE_WRITE;
    }

    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    return &pt[pt_idx];
}

/*
 * Initialize's virtual memory manager
 */
void vmm_init(void)
{
}

/*
 * Map's virtual address to physical
 */
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t *pte = get_pte(virt, 1);

    if (!pte)
    {
        printf("[VMM] Failed to map 0x%llx\n", virt);
        return;
    }

    *pte = (phys & ~0xFFF) | flags | PAGE_PRESENT;

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/*
 * Unmap's virtual address
 */
void vmm_unmap(uint64_t virt)
{
    uint64_t *pte = get_pte(virt, 0);

    if (!pte)
    {
        return;
    }

    *pte = 0;

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/*
 * Allocate's virtual pages
 */
void *vmm_alloc(size_t pages, uint64_t flags)
{
    void *phys = pmm_alloc_pages(pages);
    if (!phys)
    {
        return NULL;
    }

    uint64_t virt_start = next_virt_addr;

    for (size_t i = 0; i < pages; i++)
    {
        uint64_t virt = next_virt_addr + (i * PAGE_SIZE);
        uint64_t phys_addr = (uint64_t)phys + (i * PAGE_SIZE);

        vmm_map(virt, phys_addr, flags);
    }

    next_virt_addr += pages * PAGE_SIZE;

    return (void *)virt_start;
}

/*
 * Free's virtual pages
 */
void vmm_free(void *virt, size_t pages)
{
    for (size_t i = 0; i < pages; i++)
    {
        uint64_t virt_addr = (uint64_t)virt + (i * PAGE_SIZE);
        uint64_t phys = vmm_get_phys(virt_addr);

        if (phys)
        {
            pmm_free((void *)phys);
        }

        vmm_unmap(virt_addr);
    }
}

/*
 * Get's physical address from virtual
 */
uint64_t vmm_get_phys(uint64_t virt)
{
    uint64_t *pte = get_pte(virt, 0);

    if (!pte || !(*pte & PAGE_PRESENT))
    {
        return 0;
    }

    return (*pte & ~0xFFF) | (virt & 0xFFF);
}