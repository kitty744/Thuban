/*
 * Copyright (c) 2026 Trollycat
 * Block Device Layer implementation
 */

#include <thuban/blkdev.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/spinlock.h>

/* List of registered block devices */
static struct block_device *blkdev_head = NULL;

/* Spinlock to protect device list */
static spinlock_t blkdev_lock = SPINLOCK_INIT_NAMED("blkdev");

/* Next major device number to assign */
static uint32_t next_major = 1;

/*
 * Initialize block device subsystem
 */
void blkdev_init(void)
{
    spin_lock_init(&blkdev_lock, "blkdev");
    blkdev_head = NULL;
    next_major = 1;
}

/*
 * Register a block device
 */
int blkdev_register(struct block_device *dev)
{
    if (!dev || !dev->ops)
    {
        printf("[BLKDEV] Error: Invalid device or operations\n");
        return -1;
    }

    /* Validate required operations */
    if (!dev->ops->read)
    {
        printf("[BLKDEV] Error: Device %s missing read operation\n", dev->name);
        return -1;
    }

    spin_lock(&blkdev_lock);

    /* Check if device name already exists */
    struct block_device *curr = blkdev_head;
    while (curr)
    {
        if (strcmp(curr->name, dev->name) == 0)
        {
            spin_unlock(&blkdev_lock);
            printf("[BLKDEV] Error: Device %s already registered\n", dev->name);
            return -1;
        }
        curr = curr->next;
    }

    /* Assign major number if not set */
    if (dev->major == 0)
    {
        dev->major = next_major++;
    }

    /* Set default sector size if not set */
    if (dev->sector_size == 0)
    {
        dev->sector_size = SECTOR_SIZE;
    }

    /* Mark device as present */
    dev->flags |= BLKDEV_FLAG_PRESENT;

    /* Add to front of list */
    dev->next = blkdev_head;
    blkdev_head = dev;

    spin_unlock(&blkdev_lock);

    printf("[BLKDEV] Registered %s: %llu sectors (%llu MB)\n",
           dev->name, dev->total_sectors, blkdev_size_mb(dev));

    return 0;
}

/*
 * Unregister a block device
 */
void blkdev_unregister(struct block_device *dev)
{
    if (!dev)
    {
        return;
    }

    spin_lock(&blkdev_lock);

    /* Find and remove from list */
    struct block_device **curr = &blkdev_head;
    while (*curr)
    {
        if (*curr == dev)
        {
            *curr = dev->next;
            dev->next = NULL;
            dev->flags &= ~BLKDEV_FLAG_PRESENT;
            spin_unlock(&blkdev_lock);
            printf("[BLKDEV] Unregistered %s\n", dev->name);
            return;
        }
        curr = &(*curr)->next;
    }

    spin_unlock(&blkdev_lock);
    printf("[BLKDEV] Warning: Device %s not found for unregister\n", dev->name);
}

/*
 * Find block device by name
 */
struct block_device *blkdev_find(const char *name)
{
    if (!name)
    {
        return NULL;
    }

    spin_lock(&blkdev_lock);

    struct block_device *curr = blkdev_head;
    while (curr)
    {
        if (strcmp(curr->name, name) == 0)
        {
            spin_unlock(&blkdev_lock);
            return curr;
        }
        curr = curr->next;
    }

    spin_unlock(&blkdev_lock);
    return NULL;
}

/*
 * Read sectors from block device
 */
int blkdev_read(struct block_device *dev, uint64_t sector,
                uint32_t count, void *buffer)
{
    if (!dev || !buffer || count == 0)
    {
        return -1;
    }

    /* Check if device is present and readable */
    if (!(dev->flags & BLKDEV_FLAG_PRESENT))
    {
        printf("[BLKDEV] Error: Device %s not present\n", dev->name);
        return -1;
    }

    /* Check if sector is within bounds */
    if (sector >= dev->total_sectors)
    {
        printf("[BLKDEV] Error: Sector %llu out of bounds (max %llu)\n",
               sector, dev->total_sectors);
        return -1;
    }

    /* Clamp count to device size */
    if (sector + count > dev->total_sectors)
    {
        count = dev->total_sectors - sector;
    }

    /* Call driver's read function */
    if (!dev->ops || !dev->ops->read)
    {
        printf("[BLKDEV] Error: Device %s has no read operation\n", dev->name);
        return -1;
    }

    return dev->ops->read(dev, sector, count, buffer);
}

/*
 * Write sectors to block device
 */
int blkdev_write(struct block_device *dev, uint64_t sector,
                 uint32_t count, const void *buffer)
{
    if (!dev || !buffer || count == 0)
    {
        return -1;
    }

    /* Check if device is present and writable */
    if (!(dev->flags & BLKDEV_FLAG_PRESENT))
    {
        printf("[BLKDEV] Error: Device %s not present\n", dev->name);
        return -1;
    }

    if (dev->flags & BLKDEV_FLAG_READONLY)
    {
        printf("[BLKDEV] Error: Device %s is read-only\n", dev->name);
        return -1;
    }

    /* Check if sector is within bounds */
    if (sector >= dev->total_sectors)
    {
        printf("[BLKDEV] Error: Sector %llu out of bounds (max %llu)\n",
               sector, dev->total_sectors);
        return -1;
    }

    /* Clamp count to device size */
    if (sector + count > dev->total_sectors)
    {
        count = dev->total_sectors - sector;
    }

    /* Call driver's write function */
    if (!dev->ops || !dev->ops->write)
    {
        printf("[BLKDEV] Error: Device %s has no write operation\n", dev->name);
        return -1;
    }

    return dev->ops->write(dev, sector, count, buffer);
}

/*
 * Flush any cached writes
 */
int blkdev_flush(struct block_device *dev)
{
    if (!dev)
    {
        return -1;
    }

    if (!(dev->flags & BLKDEV_FLAG_PRESENT))
    {
        return -1;
    }

    /* Call driver's flush function if available */
    if (dev->ops && dev->ops->flush)
    {
        return dev->ops->flush(dev);
    }

    return 0; /* No-op if flush not implemented */
}

/*
 * List all registered block devices
 */
void blkdev_list(void)
{
    spin_lock(&blkdev_lock);

    if (!blkdev_head)
    {
        printf("No block devices registered\n");
        spin_unlock(&blkdev_lock);
        return;
    }

    printf("Block devices:\n");
    printf("%-10s %-6s %-12s %-10s %s\n",
           "Name", "Major", "Sectors", "Size", "Flags");
    printf("-------------------------------------------------------\n");

    struct block_device *curr = blkdev_head;
    while (curr)
    {
        char flags[32] = "";
        if (curr->flags & BLKDEV_FLAG_PRESENT)
            strcat(flags, "P");
        if (curr->flags & BLKDEV_FLAG_READONLY)
            strcat(flags, "R");
        if (curr->flags & BLKDEV_FLAG_REMOVABLE)
            strcat(flags, "M");

        printf("%-10s %-6u %-12llu %-10llu %s\n",
               curr->name,
               curr->major,
               curr->total_sectors,
               blkdev_size_mb(curr),
               flags);

        curr = curr->next;
    }

    spin_unlock(&blkdev_lock);
}