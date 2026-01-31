/*
 * Copyright (c) 2026 Trollycat
 * FAT32 Filesystem Driver
 */

#ifndef THUBAN_FAT32_H
#define THUBAN_FAT32_H

#include <stdint.h>
#include <thuban/vfs.h>
#include <thuban/blkdev.h>

typedef struct __attribute__((packed))
{
    uint8_t jmp[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} fat32_boot_sector_t;

typedef struct __attribute__((packed))
{
    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} fat32_dir_entry_t;

typedef struct __attribute__((packed))
{
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t first_cluster;
    uint16_t name3[2];
} fat32_lfn_entry_t;

#define FAT32_ATTR_READ_ONLY 0x01
#define FAT32_ATTR_HIDDEN 0x02
#define FAT32_ATTR_SYSTEM 0x04
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20
#define FAT32_ATTR_LFN 0x0F

#define FAT32_FREE_CLUSTER 0x00000000
#define FAT32_RESERVED_MIN 0x0FFFFFF0
#define FAT32_BAD_CLUSTER 0x0FFFFFF7
#define FAT32_EOC_MIN 0x0FFFFFF8
#define FAT32_EOC_MAX 0x0FFFFFFF

typedef struct
{
    struct block_device *dev;
    fat32_boot_sector_t boot;
    uint32_t fat_offset;
    uint32_t data_offset;
    uint32_t root_cluster;
    uint32_t total_clusters;
    uint32_t cluster_size;
    uint32_t *fat_cache;
    uint32_t fat_cache_size;
    int fat_dirty;
} fat32_fs_t;

typedef struct
{
    uint32_t first_cluster;
    uint32_t dir_cluster;
    uint32_t dir_offset;
} fat32_inode_t;

int fat32_init(void);
vfs_superblock_t *fat32_mount(const char *dev, uint32_t flags);
void fat32_unmount(vfs_superblock_t *sb);
vfs_node_t *fat32_lookup(vfs_node_t *dir, const char *name);
int fat32_create(vfs_node_t *dir, const char *name, mode_t mode);
int fat32_mkdir(vfs_node_t *dir, const char *name, mode_t mode);
int fat32_rmdir(vfs_node_t *dir, const char *name);
int fat32_unlink(vfs_node_t *dir, const char *name);
int fat32_open(vfs_node_t *node, vfs_file_t *file);
int fat32_close(vfs_node_t *node, vfs_file_t *file);
ssize_t fat32_read(vfs_file_t *file, void *buf, size_t count, off_t offset);
ssize_t fat32_write(vfs_file_t *file, const void *buf, size_t count, off_t offset);
int fat32_readdir(vfs_file_t *file, struct dirent *dirent, size_t count);
uint32_t fat32_get_next_cluster(fat32_fs_t *fs, uint32_t cluster);
uint32_t fat32_alloc_cluster(fat32_fs_t *fs);
void fat32_free_cluster(fat32_fs_t *fs, uint32_t cluster);
void fat32_free_chain(fat32_fs_t *fs, uint32_t start_cluster);
uint32_t fat32_cluster_to_sector(fat32_fs_t *fs, uint32_t cluster);
int fat32_read_cluster(fat32_fs_t *fs, uint32_t cluster, void *buf);
int fat32_write_cluster(fat32_fs_t *fs, uint32_t cluster, const void *buf);
void fat32_name_to_83(const char *name, char *name83);
void fat32_83_to_name(const char *name83, char *name);
int fat32_is_valid_name(const char *name);
uint16_t fat32_encode_time(time_t t);
uint16_t fat32_encode_date(time_t t);
time_t fat32_decode_datetime(uint16_t date, uint16_t time);

#endif /* THUBAN_FAT32_H */