/*
 * Copyright (c) 2026 Trollycat
 * Paging implementation
 */

#include <thuban/paging.h>
#include <thuban/stdio.h>

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000ULL

extern uint64_t p4_table;

/*
 * Initialize's paging system
 * NOTE: Paging is already set up by boot.s so this just confirms it
 */
void paging_init(void)
{
}

/*
 * Map's a virtual page to physical page
 */
void paging_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t *pml4 = &p4_table;

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    // ensure all levels exist
    if (!(pml4[pml4_idx] & PAGING_PRESENT))
    {
        return; // should use vmm for allocation
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGING_PRESENT))
    {
        return;
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pd[pd_idx] & PAGING_PRESENT))
    {
        return;
    }

    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    pt[pt_idx] = (phys & ~0xFFF) | flags | PAGING_PRESENT;

    paging_invalidate(virt);
}

/*
 * Unmap's a virtual page
 */
void paging_unmap(uint64_t virt)
{
    uint64_t *pml4 = &p4_table;

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PAGING_PRESENT))
    {
        return;
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGING_PRESENT))
    {
        return;
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pd[pd_idx] & PAGING_PRESENT))
    {
        return;
    }

    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    pt[pt_idx] = 0;

    paging_invalidate(virt);
}

/*
 * Get's physical address from virtual
 */
uint64_t paging_get_phys(uint64_t virt)
{
    uint64_t *pml4 = &p4_table;

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PAGING_PRESENT))
    {
        return 0;
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGING_PRESENT))
    {
        return 0;
    }

    // check for 1GB page
    if (pdpt[pdpt_idx] & PAGING_HUGE)
    {
        return (pdpt[pdpt_idx] & ~0x3FFFFFFF) | (virt & 0x3FFFFFFF);
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pd[pd_idx] & PAGING_PRESENT))
    {
        return 0;
    }

    // check for 2MB page (like our boot mapping)
    if (pd[pd_idx] & PAGING_HUGE)
    {
        return (pd[pd_idx] & ~0x1FFFFF) | (virt & 0x1FFFFF);
    }

    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ~0xFFF) + KERNEL_VIRT_BASE);

    if (!(pt[pt_idx] & PAGING_PRESENT))
    {
        return 0;
    }

    return (pt[pt_idx] & ~0xFFF) | (virt & 0xFFF);
}

/*
 * Invalidate's TLB entry for virtual address
 */
void paging_invalidate(uint64_t virt)
{
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}