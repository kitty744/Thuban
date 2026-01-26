/*
 * Copyright (c) 2026 Trollycat
 * Multiboot2 parser for Thuban
 */

#ifndef THUBAN_MULTIBOOT_H
#define THUBAN_MULTIBOOT_H

#include <stdint.h>
#include <stddef.h>

// multiboot2 magic number
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// multiboot2 tag types
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

// memory map entry types
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_string
{
    uint32_t type;
    uint32_t size;
    char string[0];
};

struct multiboot_tag_basic_meminfo
{
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
};

struct multiboot_mmap_entry
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct multiboot_tag_mmap
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[0];
};

struct multiboot_info
{
    uint64_t total_mem;
    uint64_t available_mem;
    uint64_t kernel_start;
    uint64_t kernel_end;
    const char *bootloader_name;
    const char *cmdline;
};

// parse multiboot info
void multiboot_parse(uint32_t magic, void *mbi_ptr);

// get parsed multiboot info
struct multiboot_info *multiboot_get_info(void);

#endif