/*
 * Copyright (c) 2026 Trollycat
 * GDT implementation
 */

#include <thuban/gdt.h>
#include <thuban/stdio.h>
#include <thuban/string.h>

#define GDT_ENTRIES 7

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_pointer;
static struct tss_entry tss;

/*
 * Set's a GDT entry
 */
static void gdt_set_gate(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

/*
 * Set's a TSS entry in GDT
 * NOTE: TSS is 16 bytes in 64-bit mode so we need two entries
 */
static void gdt_set_tss(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_set_gate(num, base, limit, access, gran);

    // upper 8 bytes of TSS descriptor
    gdt[num + 1].limit_low = (base >> 32) & 0xFFFF;
    gdt[num + 1].base_low = (base >> 48) & 0xFFFF;
    gdt[num + 1].base_middle = 0;
    gdt[num + 1].access = 0;
    gdt[num + 1].granularity = 0;
    gdt[num + 1].base_high = 0;
}

/*
 * Initialize's the GDT
 */
void gdt_init(void)
{
    gdt_pointer.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_pointer.base = (uint64_t)&gdt;

    // null descriptor (0x00)
    gdt_set_gate(0, 0, 0, 0, 0);

    // kernel code segment (0x08)
    // base=0, limit=0xFFFFFFFF, access=0x9A (present, ring 0, code, executable, readable)
    // granularity=0xA0 (64-bit, 4KB granularity)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);

    // kernel data segment (0x10)
    // base=0, limit=0xFFFFFFFF, access=0x92 (present, ring 0, data, writable)
    // granularity=0xC0 (4KB granularity)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xC0);

    // user code segment (0x18)
    // base=0, limit=0xFFFFFFFF, access=0xFA (present, ring 3, code, executable, readable)
    // granularity=0xA0 (64-bit, 4KB granularity)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xA0);

    // user data segment (0x20)
    // base=0, limit=0xFFFFFFFF, access=0xF2 (present, ring 3, data, writable)
    // granularity=0xC0 (4KB granularity)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xC0);

    // TSS (0x28)
    memset(&tss, 0, sizeof(struct tss_entry));
    tss.iomap_base = sizeof(struct tss_entry);

    gdt_set_tss(5, (uint64_t)&tss, sizeof(struct tss_entry), 0x89, 0x00);

    gdt_flush((uint64_t)&gdt_pointer);
    tss_flush();
}

/*
 * Set's kernel stack pointer in TSS
 * NOTE: Used when switching from user mode to kernel mode
 */
void gdt_set_kernel_stack(uint64_t stack)
{
    tss.rsp0 = stack;
}