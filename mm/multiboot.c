/*
 * Copyright (c) 2026 Trollycat
 * Multiboot2 parser implementation
 */

#include <thuban/multiboot.h>
#include <thuban/stdio.h>
#include <thuban/string.h>

static struct multiboot_info mbi_info;

/*
 * Parse's multiboot2 information structure
 */
void multiboot_parse(uint32_t magic, void *mbi_ptr)
{
    if (magic != 0x36d76289)
    {
        printf("[MULTIBOOT] Invalid magic: 0x%x\n", magic);
        mbi_info.total_mem = 512 * 1024 * 1024; // fallback to 512MB
        mbi_info.available_mem = 512 * 1024 * 1024;
        return;
    }

    memset(&mbi_info, 0, sizeof(struct multiboot_info));

    struct multiboot_tag *tag = (struct multiboot_tag *)((uint8_t *)mbi_ptr + 8);

    while (tag->type != MULTIBOOT_TAG_TYPE_END)
    {
        switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
        {
            struct multiboot_tag_string *cmdline_tag = (struct multiboot_tag_string *)tag;
            mbi_info.cmdline = cmdline_tag->string;
            break;
        }
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
        {
            struct multiboot_tag_string *bootloader_tag = (struct multiboot_tag_string *)tag;
            mbi_info.bootloader_name = bootloader_tag->string;
            break;
        }
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
        {
            struct multiboot_tag_basic_meminfo *meminfo = (struct multiboot_tag_basic_meminfo *)tag;
            mbi_info.total_mem = ((uint64_t)meminfo->mem_upper + 1024) * 1024;
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP:
        {
            struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *)tag;

            for (struct multiboot_mmap_entry *entry = mmap->entries;
                 (uint8_t *)entry < (uint8_t *)mmap + mmap->size;
                 entry = (struct multiboot_mmap_entry *)((uint8_t *)entry + mmap->entry_size))
            {
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    mbi_info.available_mem += entry->len;
                }
            }
            break;
        }
        default:
            break;
        }

        // align to 8 byte boundary
        tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }
}

/*
 * Get's parsed multiboot info
 */
struct multiboot_info *multiboot_get_info(void)
{
    return &mbi_info;
}