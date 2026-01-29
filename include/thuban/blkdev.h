/*
 * Copyright (c) 2026 Trollycat
 * Block Device Layer - Generic interface for block devices
 *
 * Provides abstraction layer for storage devices (ATA, AHCI, NVMe, etc.)
 */

#ifndef THUBAN_BLKDEV_H
#define THUBAN_BLKDEV_H

#include <stdint.h>
#include <stddef.h>

/* Standard sector size (512 bytes) */
#define SECTOR_SIZE 512

/* Block device flags */
#define BLKDEV_FLAG_REMOVABLE (1 << 0) /* Device is removable (USB, CD-ROM) */
#define BLKDEV_FLAG_READONLY (1 << 1)  /* Device is read-only */
#define BLKDEV_FLAG_PRESENT (1 << 2)   /* Device is present and ready */

/* Block device types */
#define BLKDEV_TYPE_DISK 0    /* Hard disk */
#define BLKDEV_TYPE_CDROM 1   /* CD-ROM */
#define BLKDEV_TYPE_FLOPPY 2  /* Floppy disk */
#define BLKDEV_TYPE_RAMDISK 3 /* RAM disk */

/* Forward declarations */
struct block_device;
struct block_request;

/*
 * Block device operations
 * Each driver implements these operations
 */
struct block_device_ops
{
    /* Read sectors from device */
    int (*read)(struct block_device *dev, uint64_t sector,
                uint32_t count, void *buffer);

    /* Write sectors to device */
    int (*write)(struct block_device *dev, uint64_t sector,
                 uint32_t count, const void *buffer);

    /* Flush any cached writes */
    int (*flush)(struct block_device *dev);

    /* Get device information */
    int (*ioctl)(struct block_device *dev, uint32_t cmd, void *arg);
};

/*
 * Block device structure
 * Represents a single block device (e.g., /dev/sda)
 */
struct block_device
{
    char name[32];  /* Device name (e.g., "sda", "hda") */
    uint32_t major; /* Major device number */
    uint32_t minor; /* Minor device number */

    uint64_t total_sectors; /* Total number of sectors */
    uint32_t sector_size;   /* Sector size (usually 512) */

    uint32_t type;  /* Device type (BLKDEV_TYPE_*) */
    uint32_t flags; /* Device flags (BLKDEV_FLAG_*) */

    const struct block_device_ops *ops; /* Device operations */
    void *private_data;                 /* Driver-specific data */

    struct block_device *next; /* Linked list of devices */
};

/*
 * Initialize block device subsystem
 */
void blkdev_init(void);

/*
 * Register a block device
 *
 * Parameters:
 *   dev - Block device structure to register
 *
 * Returns:
 *   0 on success, -1 on error
 */
int blkdev_register(struct block_device *dev);

/*
 * Unregister a block device
 *
 * Parameters:
 *   dev - Block device to unregister
 */
void blkdev_unregister(struct block_device *dev);

/*
 * Find block device by name
 *
 * Parameters:
 *   name - Device name (e.g., "sda", "hda")
 *
 * Returns:
 *   Pointer to block device, or NULL if not found
 */
struct block_device *blkdev_find(const char *name);

/*
 * Read sectors from block device
 *
 * Parameters:
 *   dev - Block device
 *   sector - Starting sector number (LBA)
 *   count - Number of sectors to read
 *   buffer - Buffer to store data (must be at least count * sector_size bytes)
 *
 * Returns:
 *   Number of sectors read, or -1 on error
 */
int blkdev_read(struct block_device *dev, uint64_t sector,
                uint32_t count, void *buffer);

/*
 * Write sectors to block device
 *
 * Parameters:
 *   dev - Block device
 *   sector - Starting sector number (LBA)
 *   count - Number of sectors to write
 *   buffer - Data to write
 *
 * Returns:
 *   Number of sectors written, or -1 on error
 */
int blkdev_write(struct block_device *dev, uint64_t sector,
                 uint32_t count, const void *buffer);

/*
 * Flush any cached writes
 *
 * Parameters:
 *   dev - Block device
 *
 * Returns:
 *   0 on success, -1 on error
 */
int blkdev_flush(struct block_device *dev);

/*
 * List all registered block devices
 * Prints to console for debugging
 */
void blkdev_list(void);

/*
 * Get total size of device in bytes
 */
static inline uint64_t blkdev_size(struct block_device *dev)
{
    return dev->total_sectors * dev->sector_size;
}

/*
 * Get total size of device in KB
 */
static inline uint64_t blkdev_size_kb(struct block_device *dev)
{
    return (dev->total_sectors * dev->sector_size) / 1024;
}

/*
 * Get total size of device in MB
 */
static inline uint64_t blkdev_size_mb(struct block_device *dev)
{
    return (dev->total_sectors * dev->sector_size) / (1024 * 1024);
}

#endif /* THUBAN_BLKDEV_H */