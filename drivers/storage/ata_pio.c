/*
 * Copyright (c) 2026 Trollycat
 * ATA PIO Driver Implementation
 */

#include <thuban/ata_pio.h>
#include <thuban/blkdev.h>
#include <thuban/io.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/spinlock.h>

/* ATA device structure */
struct ata_device
{
    uint8_t bus;           /* 0 = primary, 1 = secondary */
    uint8_t drive;         /* 0 = master, 1 = slave */
    uint16_t io_base;      /* I/O base port */
    uint16_t control_base; /* Control base port */
    uint8_t exists;        /* 1 if device exists */

    uint64_t sectors; /* Total sectors */
    int lba48;        /* 1 if LBA48 supported */
    char model[41];   /* Model string */
    char serial[21];  /* Serial number */
    char firmware[9]; /* Firmware revision */

    struct block_device blkdev; /* Block device interface */
    spinlock_t lock;            /* Device lock */
};

/* Global ATA devices (4 maximum: 2 buses × 2 drives) */
static struct ata_device ata_devices[4];

/* Helper: Get device index */
static inline int ata_dev_index(uint8_t bus, uint8_t drive)
{
    return (bus * 2) + drive;
}

/* Helper: Get device */
static inline struct ata_device *ata_get_device(uint8_t bus, uint8_t drive)
{
    int idx = ata_dev_index(bus, drive);
    if (idx >= 4)
        return NULL;
    return &ata_devices[idx];
}

/*
 * Wait for ATA drive to be ready (BSY cleared)
 */
static int ata_wait_ready(uint16_t io_base, uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 1000; /* Approximate */

    while (timeout--)
    {
        uint8_t status = inb(io_base + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY))
        {
            return 0; /* Ready */
        }
    }

    return -1; /* Timeout */
}

/*
 * Wait for DRQ (data request) bit to be set
 */
static int ata_wait_drq(uint16_t io_base, uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 1000;

    while (timeout--)
    {
        uint8_t status = inb(io_base + ATA_REG_STATUS);
        if (status & ATA_STATUS_DRQ)
        {
            return 0; /* DRQ set */
        }
        if (status & ATA_STATUS_ERR)
        {
            return -1; /* Error */
        }
    }

    return -1; /* Timeout */
}

/*
 * 400ns delay by reading alternate status 4 times
 */
static void ata_delay_400ns(uint16_t control_base)
{
    for (int i = 0; i < 4; i++)
    {
        inb(control_base + ATA_REG_ALTSTATUS);
    }
}

/*
 * Select ATA drive (master or slave)
 */
static void ata_select_drive(uint16_t io_base, uint8_t drive)
{
    uint8_t value = (drive == 0) ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    outb(io_base + ATA_REG_DRIVE, value);
}

/*
 * Read 256 16-bit words from ATA data port
 */
static void ata_read_buffer(uint16_t io_base, uint16_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        buffer[i] = inw(io_base + ATA_REG_DATA);
    }
}

/*
 * Write 256 16-bit words to ATA data port
 */
static void ata_write_buffer(uint16_t io_base, const uint16_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        outw(io_base + ATA_REG_DATA, buffer[i]);
    }
}

/*
 * Fix ATA string (swap byte pairs and trim spaces)
 */
static void ata_fix_string(char *str, size_t len)
{
    /* Swap bytes */
    for (size_t i = 0; i < len; i += 2)
    {
        char tmp = str[i];
        str[i] = str[i + 1];
        str[i + 1] = tmp;
    }

    /* Trim trailing spaces */
    for (int i = len - 1; i >= 0; i--)
    {
        if (str[i] == ' ')
        {
            str[i] = '\0';
        }
        else
        {
            break;
        }
    }

    str[len] = '\0';
}

/*
 * Identify ATA device
 */
static int ata_identify(struct ata_device *dev)
{
    uint16_t io_base = dev->io_base;
    uint16_t control_base = dev->control_base;

    printf("[ATA] Trying to identify device: bus=%d, drive=%d, io=0x%x\n",
           dev->bus, dev->drive, io_base);

    /* Select drive */
    ata_select_drive(io_base, dev->drive);
    ata_delay_400ns(control_base);

    /* Clear sector count and LBA registers */
    outb(io_base + ATA_REG_SECCOUNT, 0);
    outb(io_base + ATA_REG_LBALO, 0);
    outb(io_base + ATA_REG_LBAMID, 0);
    outb(io_base + ATA_REG_LBAHI, 0);

    /* Send IDENTIFY command */
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay_400ns(control_base);

    /* Check if device exists */
    uint8_t status = inb(io_base + ATA_REG_STATUS);
    printf("[ATA]   Initial status: 0x%02x\n", status);

    if (status == 0)
    {
        printf("[ATA]   No device (status = 0)\n");
        return -1; /* No device */
    }

    if (status == 0xFF)
    {
        printf("[ATA]   Floating bus (status = 0xFF)\n");
        return -1; /* Floating bus */
    }

    /* Wait for BSY to clear */
    if (ata_wait_ready(io_base, 1000) != 0)
    {
        return -1; /* Timeout */
    }

    /* Check for errors */
    uint8_t lbamid = inb(io_base + ATA_REG_LBAMID);
    uint8_t lbahi = inb(io_base + ATA_REG_LBAHI);
    if (lbamid != 0 || lbahi != 0)
    {
        return -1; /* Not ATA device (might be ATAPI) */
    }

    /* Wait for DRQ */
    if (ata_wait_drq(io_base, 1000) != 0)
    {
        return -1; /* Timeout */
    }

    /* Read identification data */
    struct ata_identify identify;
    ata_read_buffer(io_base, (uint16_t *)&identify, 256);

    /* Extract information */
    memcpy(dev->model, identify.model, 40);
    ata_fix_string(dev->model, 40);

    memcpy(dev->serial, identify.serial, 20);
    ata_fix_string(dev->serial, 20);

    memcpy(dev->firmware, identify.firmware, 8);
    ata_fix_string(dev->firmware, 8);

    /* Get sector count */
    if (identify.lba48_sectors > 0)
    {
        dev->sectors = identify.lba48_sectors;
        dev->lba48 = 1;
    }
    else
    {
        dev->sectors = identify.lba28_sectors;
        dev->lba48 = 0;
    }

    return 0;
}

/*
 * Read sectors using LBA28
 */
static int ata_read_lba28(struct ata_device *dev, uint32_t lba,
                          uint8_t count, void *buffer)
{
    uint16_t io_base = dev->io_base;
    uint16_t control_base = dev->control_base;

    spin_lock(&dev->lock);

    /* Wait for drive to be ready */
    if (ata_wait_ready(io_base, 1000) != 0)
    {
        spin_unlock(&dev->lock);
        printf("[ATA] Timeout waiting for ready\n");
        return -1;
    }

    /* Select drive and set LBA mode */
    uint8_t drive_bits = (dev->drive == 0) ? 0xE0 : 0xF0;
    outb(io_base + ATA_REG_DRIVE, drive_bits | ((lba >> 24) & 0x0F));

    /* Set sector count */
    outb(io_base + ATA_REG_SECCOUNT, count);

    /* Set LBA */
    outb(io_base + ATA_REG_LBALO, (uint8_t)(lba));
    outb(io_base + ATA_REG_LBAMID, (uint8_t)(lba >> 8));
    outb(io_base + ATA_REG_LBAHI, (uint8_t)(lba >> 16));

    /* Send READ command */
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    ata_delay_400ns(control_base);

    /* Read sectors */
    uint16_t *buf = (uint16_t *)buffer;
    for (int i = 0; i < count; i++)
    {
        /* Wait for DRQ */
        if (ata_wait_drq(io_base, 1000) != 0)
        {
            spin_unlock(&dev->lock);
            printf("[ATA] Timeout waiting for DRQ\n");
            return -1;
        }

        /* Read 256 words (512 bytes) */
        ata_read_buffer(io_base, buf, 256);
        buf += 256;
    }

    spin_unlock(&dev->lock);
    return 0;
}

/*
 * Write sectors using LBA28
 */
static int ata_write_lba28(struct ata_device *dev, uint32_t lba,
                           uint8_t count, const void *buffer)
{
    uint16_t io_base = dev->io_base;
    uint16_t control_base = dev->control_base;

    spin_lock(&dev->lock);

    /* Wait for drive to be ready */
    if (ata_wait_ready(io_base, 1000) != 0)
    {
        spin_unlock(&dev->lock);
        return -1;
    }

    /* Select drive and set LBA mode */
    uint8_t drive_bits = (dev->drive == 0) ? 0xE0 : 0xF0;
    outb(io_base + ATA_REG_DRIVE, drive_bits | ((lba >> 24) & 0x0F));

    /* Set sector count */
    outb(io_base + ATA_REG_SECCOUNT, count);

    /* Set LBA */
    outb(io_base + ATA_REG_LBALO, (uint8_t)(lba));
    outb(io_base + ATA_REG_LBAMID, (uint8_t)(lba >> 8));
    outb(io_base + ATA_REG_LBAHI, (uint8_t)(lba >> 16));

    /* Send WRITE command */
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ata_delay_400ns(control_base);

    /* Write sectors */
    const uint16_t *buf = (const uint16_t *)buffer;
    for (int i = 0; i < count; i++)
    {
        /* Wait for DRQ */
        if (ata_wait_drq(io_base, 1000) != 0)
        {
            spin_unlock(&dev->lock);
            return -1;
        }

        /* Write 256 words (512 bytes) */
        ata_write_buffer(io_base, buf, 256);
        buf += 256;
    }

    /* Flush cache */
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_ready(io_base, 1000);

    spin_unlock(&dev->lock);
    return 0;
}

/*
 * Block device read operation
 */
static int ata_blkdev_read(struct block_device *blkdev, uint64_t sector,
                           uint32_t count, void *buffer)
{
    struct ata_device *dev = (struct ata_device *)blkdev->private_data;

    /* ATA PIO can only handle 256 sectors at a time */
    while (count > 0)
    {
        uint8_t chunk = (count > 256) ? 255 : count;

        if (ata_read_lba28(dev, sector, chunk, buffer) != 0)
        {
            return -1;
        }

        sector += chunk;
        count -= chunk;
        buffer = (uint8_t *)buffer + (chunk * 512);
    }

    return 0;
}

/*
 * Block device write operation
 */
static int ata_blkdev_write(struct block_device *blkdev, uint64_t sector,
                            uint32_t count, const void *buffer)
{
    struct ata_device *dev = (struct ata_device *)blkdev->private_data;

    /* ATA PIO can only handle 256 sectors at a time */
    while (count > 0)
    {
        uint8_t chunk = (count > 256) ? 255 : count;

        if (ata_write_lba28(dev, sector, chunk, buffer) != 0)
        {
            return -1;
        }

        sector += chunk;
        count -= chunk;
        buffer = (const uint8_t *)buffer + (chunk * 512);
    }

    return 0;
}

/* Block device operations */
static const struct block_device_ops ata_blkdev_ops = {
    .read = ata_blkdev_read,
    .write = ata_blkdev_write,
    .flush = NULL,
    .ioctl = NULL,
};

/*
 * Initialize ATA PIO driver
 */
void ata_pio_init(void)
{
    /* Initialize device structures */
    memset(ata_devices, 0, sizeof(ata_devices));

    /* Set up bus parameters */
    for (int bus = 0; bus < 2; bus++)
    {
        uint16_t io_base = (bus == 0) ? ATA_PRIMARY_IO : ATA_SECONDARY_IO;
        uint16_t control_base = (bus == 0) ? ATA_PRIMARY_CONTROL : ATA_SECONDARY_CONTROL;

        for (int drive = 0; drive < 2; drive++)
        {
            struct ata_device *dev = ata_get_device(bus, drive);
            dev->bus = bus;
            dev->drive = drive;
            dev->io_base = io_base;
            dev->control_base = control_base;
            dev->exists = 0;

            spin_lock_init(&dev->lock, "ata_device");

            /* Try to identify device */
            if (ata_identify(dev) == 0)
            {
                dev->exists = 1;

                printf("[ATA] Found %s on %s bus, %s drive\n",
                       dev->model,
                       (bus == 0) ? "primary" : "secondary",
                       (drive == 0) ? "master" : "slave");
                printf("[ATA]   Serial: %s, Firmware: %s\n",
                       dev->serial, dev->firmware);
                printf("[ATA]   Capacity: %llu sectors (%llu MB)\n",
                       dev->sectors, (dev->sectors * 512) / (1024 * 1024));

                /* Register as block device */
                const char *names[] = {"hda", "hdb", "hdc", "hdd"};
                int idx = ata_dev_index(bus, drive);

                struct block_device *blkdev = &dev->blkdev;
                strncpy(blkdev->name, names[idx], sizeof(blkdev->name) - 1);
                blkdev->major = 3; /* ATA major number */
                blkdev->minor = idx;
                blkdev->total_sectors = dev->sectors;
                blkdev->sector_size = 512;
                blkdev->type = BLKDEV_TYPE_DISK;
                blkdev->flags = 0;
                blkdev->ops = &ata_blkdev_ops;
                blkdev->private_data = dev;

                blkdev_register(blkdev);
            }
        }
    }
}

/*
 * Public API: Read sectors
 */
int ata_pio_read(uint8_t bus, uint8_t drive, uint64_t lba,
                 uint32_t count, void *buffer)
{
    struct ata_device *dev = ata_get_device(bus, drive);
    if (!dev || !dev->exists)
    {
        return -1;
    }

    return ata_blkdev_read(&dev->blkdev, lba, count, buffer);
}

/*
 * Public API: Write sectors
 */
int ata_pio_write(uint8_t bus, uint8_t drive, uint64_t lba,
                  uint32_t count, const void *buffer)
{
    struct ata_device *dev = ata_get_device(bus, drive);
    if (!dev || !dev->exists)
    {
        return -1;
    }

    return ata_blkdev_write(&dev->blkdev, lba, count, buffer);
}