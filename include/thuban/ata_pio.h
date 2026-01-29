/*
 * Copyright (c) 2026 Trollycat
 * ATA PIO (Programmed I/O) Driver
 *
 * Simple ATA driver using PIO mode (no DMA, no interrupts)
 * Supports IDE/PATA hard disks
 */

#ifndef THUBAN_ATA_PIO_H
#define THUBAN_ATA_PIO_H

#include <stdint.h>

/* ATA I/O Ports - Primary Bus */
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_PRIMARY_IRQ 14

/* ATA I/O Ports - Secondary Bus */
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CONTROL 0x376
#define ATA_SECONDARY_IRQ 15

/* ATA Register Offsets */
#define ATA_REG_DATA 0     /* Data register (16-bit) */
#define ATA_REG_ERROR 1    /* Error register (read) */
#define ATA_REG_FEATURES 1 /* Features register (write) */
#define ATA_REG_SECCOUNT 2 /* Sector count */
#define ATA_REG_LBALO 3    /* LBA low byte */
#define ATA_REG_LBAMID 4   /* LBA mid byte */
#define ATA_REG_LBAHI 5    /* LBA high byte */
#define ATA_REG_DRIVE 6    /* Drive/Head register */
#define ATA_REG_STATUS 7   /* Status register (read) */
#define ATA_REG_COMMAND 7  /* Command register (write) */

/* ATA Control Register Offsets */
#define ATA_REG_CONTROL 0   /* Device control register */
#define ATA_REG_ALTSTATUS 0 /* Alternate status (read) */

/* ATA Commands */
#define ATA_CMD_READ_PIO 0x20      /* Read sectors with retry */
#define ATA_CMD_READ_PIO_EXT 0x24  /* Read sectors (LBA48) */
#define ATA_CMD_WRITE_PIO 0x30     /* Write sectors with retry */
#define ATA_CMD_WRITE_PIO_EXT 0x34 /* Write sectors (LBA48) */
#define ATA_CMD_CACHE_FLUSH 0xE7   /* Flush write cache */
#define ATA_CMD_IDENTIFY 0xEC      /* Identify device */

/* ATA Status Register Bits */
#define ATA_STATUS_ERR (1 << 0)  /* Error */
#define ATA_STATUS_IDX (1 << 1)  /* Index (obsolete) */
#define ATA_STATUS_CORR (1 << 2) /* Corrected data */
#define ATA_STATUS_DRQ (1 << 3)  /* Data request */
#define ATA_STATUS_DSC (1 << 4)  /* Drive seek complete */
#define ATA_STATUS_DF (1 << 5)   /* Drive fault */
#define ATA_STATUS_DRDY (1 << 6) /* Drive ready */
#define ATA_STATUS_BSY (1 << 7)  /* Busy */

/* ATA Error Register Bits */
#define ATA_ERROR_AMNF (1 << 0)  /* Address mark not found */
#define ATA_ERROR_TK0NF (1 << 1) /* Track 0 not found */
#define ATA_ERROR_ABRT (1 << 2)  /* Aborted command */
#define ATA_ERROR_MCR (1 << 3)   /* Media change request */
#define ATA_ERROR_IDNF (1 << 4)  /* ID not found */
#define ATA_ERROR_MC (1 << 5)    /* Media changed */
#define ATA_ERROR_UNC (1 << 6)   /* Uncorrectable data */
#define ATA_ERROR_BBK (1 << 7)   /* Bad block */

/* Drive selection */
#define ATA_DRIVE_MASTER 0xA0 /* Master drive */
#define ATA_DRIVE_SLAVE 0xB0  /* Slave drive */

/* ATA Device Information (from IDENTIFY command) */
struct ata_identify
{
    uint16_t config;    /* Word 0: General configuration */
    uint16_t cylinders; /* Word 1: Cylinders */
    uint16_t reserved1;
    uint16_t heads; /* Word 3: Heads */
    uint16_t reserved2[2];
    uint16_t sectors; /* Word 6: Sectors per track */
    uint16_t reserved3[3];
    char serial[20]; /* Words 10-19: Serial number */
    uint16_t reserved4[3];
    char firmware[8]; /* Words 23-26: Firmware revision */
    char model[40];   /* Words 27-46: Model number */
    uint16_t reserved5[13];
    uint32_t lba28_sectors; /* Words 60-61: LBA28 total sectors */
    uint16_t reserved6[38];
    uint64_t lba48_sectors; /* Words 100-103: LBA48 total sectors */
    uint16_t reserved7[152];
};

/*
 * Initialize ATA PIO driver
 * Detects and initializes ATA drives on primary and secondary buses
 */
void ata_pio_init(void);

/*
 * Read sectors from ATA drive
 *
 * Parameters:
 *   bus - 0 for primary, 1 for secondary
 *   drive - 0 for master, 1 for slave
 *   lba - Logical Block Address
 *   count - Number of sectors to read
 *   buffer - Buffer to store data
 *
 * Returns:
 *   0 on success, -1 on error
 */
int ata_pio_read(uint8_t bus, uint8_t drive, uint64_t lba,
                 uint32_t count, void *buffer);

/*
 * Write sectors to ATA drive
 *
 * Parameters:
 *   bus - 0 for primary, 1 for secondary
 *   drive - 0 for master, 1 for slave
 *   lba - Logical Block Address
 *   count - Number of sectors to write
 *   buffer - Data to write
 *
 * Returns:
 *   0 on success, -1 on error
 */
int ata_pio_write(uint8_t bus, uint8_t drive, uint64_t lba,
                  uint32_t count, const void *buffer);

#endif /* THUBAN_ATA_PIO_H */