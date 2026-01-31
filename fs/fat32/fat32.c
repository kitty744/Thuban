/*
 * Copyright (c) 2026 Trollycat
 * FAT32 Filesystem Driver Implementation
 */

#include <thuban/fat32.h>
#include <thuban/vfs.h>
#include <thuban/string.h>
#include <thuban/stdio.h>
#include <thuban/heap.h>
#include <thuban/spinlock.h>
#include <thuban/blkdev.h>

static vfs_file_operations_t fat32_file_ops;
static vfs_inode_operations_t fat32_inode_ops;
static vfs_superblock_operations_t fat32_sb_ops;
static vfs_filesystem_t fat32_filesystem;
static spinlock_t fat32_lock;

uint32_t fat32_cluster_to_sector(fat32_fs_t *fs, uint32_t cluster)
{
    if (cluster < 2)
    {
        return 0;
    }
    return fs->data_offset + (cluster - 2) * fs->boot.sectors_per_cluster;
}

int fat32_read_cluster(fat32_fs_t *fs, uint32_t cluster, void *buf)
{
    uint32_t sector = fat32_cluster_to_sector(fs, cluster);
    if (sector == 0)
    {
        return -1;
    }

    for (uint32_t i = 0; i < fs->boot.sectors_per_cluster; i++)
    {
        if (blkdev_read(fs->dev, sector + i, 1,
                        (uint8_t *)buf + (i * fs->boot.bytes_per_sector)) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int fat32_write_cluster(fat32_fs_t *fs, uint32_t cluster, const void *buf)
{
    uint32_t sector = fat32_cluster_to_sector(fs, cluster);
    if (sector == 0)
    {
        return -1;
    }

    for (uint32_t i = 0; i < fs->boot.sectors_per_cluster; i++)
    {
        if (blkdev_write(fs->dev, sector + i, 1,
                         (const uint8_t *)buf + (i * fs->boot.bytes_per_sector)) != 0)
        {
            return -1;
        }
    }

    return 0;
}

uint32_t fat32_get_next_cluster(fat32_fs_t *fs, uint32_t cluster)
{
    if (cluster < 2 || cluster >= fs->total_clusters + 2)
    {
        return FAT32_EOC_MAX;
    }

    if (fs->fat_cache && cluster < fs->fat_cache_size)
    {
        uint32_t next = fs->fat_cache[cluster] & 0x0FFFFFFF;
        return next;
    }

    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_offset + (fat_offset / fs->boot.bytes_per_sector);
    uint32_t ent_offset = fat_offset % fs->boot.bytes_per_sector;

    uint8_t sector_buf[512];
    if (blkdev_read(fs->dev, fat_sector, 1, sector_buf) != 0)
    {
        return FAT32_EOC_MAX;
    }

    uint32_t *fat_entry = (uint32_t *)(sector_buf + ent_offset);
    return (*fat_entry) & 0x0FFFFFFF;
}

static int fat32_set_fat_entry(fat32_fs_t *fs, uint32_t cluster, uint32_t value)
{
    if (cluster < 2 || cluster >= fs->total_clusters + 2)
    {
        return -1;
    }

    if (fs->fat_cache && cluster < fs->fat_cache_size)
    {
        fs->fat_cache[cluster] = value & 0x0FFFFFFF;
        fs->fat_dirty = 1;
    }

    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_offset + (fat_offset / fs->boot.bytes_per_sector);
    uint32_t ent_offset = fat_offset % fs->boot.bytes_per_sector;

    uint8_t sector_buf[512];
    if (blkdev_read(fs->dev, fat_sector, 1, sector_buf) != 0)
    {
        return -1;
    }

    uint32_t *fat_entry = (uint32_t *)(sector_buf + ent_offset);
    *fat_entry = (*fat_entry & 0xF0000000) | (value & 0x0FFFFFFF);

    if (blkdev_write(fs->dev, fat_sector, 1, sector_buf) != 0)
    {
        return -1;
    }

    if (fs->boot.num_fats > 1)
    {
        uint32_t backup_fat_sector = fat_sector + fs->boot.fat_size_32;
        blkdev_write(fs->dev, backup_fat_sector, 1, sector_buf);
    }

    return 0;
}

uint32_t fat32_alloc_cluster(fat32_fs_t *fs)
{
    spin_lock(&fat32_lock);

    for (uint32_t cluster = 2; cluster < fs->total_clusters + 2; cluster++)
    {
        uint32_t entry = fat32_get_next_cluster(fs, cluster);
        if (entry == FAT32_FREE_CLUSTER)
        {
            fat32_set_fat_entry(fs, cluster, FAT32_EOC_MAX);
            spin_unlock(&fat32_lock);
            return cluster;
        }
    }

    spin_unlock(&fat32_lock);
    return 0;
}

void fat32_free_cluster(fat32_fs_t *fs, uint32_t cluster)
{
    if (cluster < 2 || cluster >= fs->total_clusters + 2)
    {
        return;
    }

    spin_lock(&fat32_lock);
    fat32_set_fat_entry(fs, cluster, FAT32_FREE_CLUSTER);
    spin_unlock(&fat32_lock);
}

void fat32_free_chain(fat32_fs_t *fs, uint32_t start_cluster)
{
    uint32_t cluster = start_cluster;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        uint32_t next = fat32_get_next_cluster(fs, cluster);
        fat32_free_cluster(fs, cluster);
        cluster = next;
    }
}

void fat32_name_to_83(const char *name, char *name83)
{
    memset(name83, ' ', 11);

    const char *dot = strrchr(name, '.');
    int name_len = dot ? (dot - name) : strlen(name);
    int ext_len = dot ? strlen(dot + 1) : 0;

    for (int i = 0; i < 8 && i < name_len; i++)
    {
        name83[i] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 'a' + 'A') : name[i];
    }

    if (dot)
    {
        for (int i = 0; i < 3 && i < ext_len; i++)
        {
            name83[8 + i] = (dot[1 + i] >= 'a' && dot[1 + i] <= 'z') ? (dot[1 + i] - 'a' + 'A') : dot[1 + i];
        }
    }
}

void fat32_83_to_name(const char *name83, char *name)
{
    int i, j = 0;

    for (i = 0; i < 8 && name83[i] != ' '; i++)
    {
        name[j++] = (name83[i] >= 'A' && name83[i] <= 'Z') ? (name83[i] - 'A' + 'a') : name83[i];
    }

    if (name83[8] != ' ')
    {
        name[j++] = '.';
        for (i = 8; i < 11 && name83[i] != ' '; i++)
        {
            name[j++] = (name83[i] >= 'A' && name83[i] <= 'Z') ? (name83[i] - 'A' + 'a') : name83[i];
        }
    }

    name[j] = '\0';
}

int fat32_is_valid_name(const char *name)
{
    if (!name || strlen(name) == 0 || strlen(name) > 255)
    {
        return 0;
    }

    const char *invalid = "\\/:*?\"<>|";
    for (const char *p = name; *p; p++)
    {
        if (strchr(invalid, *p))
        {
            return 0;
        }
    }

    return 1;
}

uint16_t fat32_encode_time(time_t t)
{
    return 0;
}

uint16_t fat32_encode_date(time_t t)
{
    return 0;
}

time_t fat32_decode_datetime(uint16_t date, uint16_t time)
{
    return 0;
}

vfs_node_t *fat32_lookup(vfs_node_t *dir, const char *name)
{
    if (!dir || !name || !vfs_is_directory(dir))
    {
        return NULL;
    }

    fat32_fs_t *fs = (fat32_fs_t *)dir->sb->fs_data;
    fat32_inode_t *dir_inode = (fat32_inode_t *)dir->fs_data;

    if (!fs || !dir_inode)
    {
        return NULL;
    }

    uint32_t cluster = dir_inode->first_cluster;
    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        return NULL;
    }

    while (cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return NULL;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int num_entries = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num_entries; i++)
        {
            if (entries[i].name[0] == 0x00)
            {
                free(cluster_buf);
                return NULL;
            }

            if ((uint8_t)entries[i].name[0] == 0xE5)
            {
                continue;
            }

            if (entries[i].attr == FAT32_ATTR_LFN)
            {
                continue;
            }

            if (entries[i].attr & FAT32_ATTR_VOLUME_ID)
            {
                continue;
            }

            char entry_name[256];
            fat32_83_to_name(entries[i].name, entry_name);

            if (strcmp(entry_name, name) == 0)
            {
                vfs_node_t *node = (vfs_node_t *)malloc(sizeof(vfs_node_t));
                if (!node)
                {
                    free(cluster_buf);
                    return NULL;
                }

                memset(node, 0, sizeof(vfs_node_t));
                strncpy(node->name, entry_name, VFS_MAX_NAME - 1);

                uint32_t first_cluster = ((uint32_t)entries[i].first_cluster_hi << 16) |
                                         entries[i].first_cluster_lo;

                node->inode = first_cluster;
                node->size = entries[i].file_size;
                node->mode = (entries[i].attr & FAT32_ATTR_DIRECTORY) ? (S_IFDIR | 0755) : (S_IFREG | 0644);
                node->uid = 0;
                node->gid = 0;
                node->nlink = 1;
                node->sb = dir->sb;
                node->parent = dir;
                node->fops = &fat32_file_ops;
                node->iops = &fat32_inode_ops;

                fat32_inode_t *inode_data = (fat32_inode_t *)malloc(sizeof(fat32_inode_t));
                if (!inode_data)
                {
                    free(node);
                    free(cluster_buf);
                    return NULL;
                }

                inode_data->first_cluster = first_cluster;
                inode_data->dir_cluster = cluster;
                inode_data->dir_offset = i;
                node->fs_data = inode_data;

                free(cluster_buf);
                return node;
            }
        }

        cluster = fat32_get_next_cluster(fs, cluster);
    }

    free(cluster_buf);
    return NULL;
}

int fat32_create(vfs_node_t *dir, const char *name, mode_t mode)
{
    if (!dir || !name || !vfs_is_directory(dir))
    {
        return -1;
    }

    if (!fat32_is_valid_name(name))
    {
        return -1;
    }

    fat32_fs_t *fs = (fat32_fs_t *)dir->sb->fs_data;
    fat32_inode_t *dir_inode = (fat32_inode_t *)dir->fs_data;

    if (!fs || !dir_inode)
    {
        return -1;
    }

    if (fat32_lookup(dir, name) != NULL)
    {
        return -1;
    }

    uint32_t new_cluster = fat32_alloc_cluster(fs);
    if (new_cluster == 0)
    {
        return -1;
    }

    uint32_t cluster = dir_inode->first_cluster;
    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        fat32_free_cluster(fs, new_cluster);
        return -1;
    }

    while (cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            fat32_free_cluster(fs, new_cluster);
            return -1;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int num_entries = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num_entries; i++)
        {
            if (entries[i].name[0] == 0x00 || (uint8_t)entries[i].name[0] == 0xE5)
            {
                memset(&entries[i], 0, sizeof(fat32_dir_entry_t));

                fat32_name_to_83(name, entries[i].name);
                entries[i].attr = (mode & S_IFDIR) ? FAT32_ATTR_DIRECTORY : 0;
                entries[i].first_cluster_hi = (new_cluster >> 16) & 0xFFFF;
                entries[i].first_cluster_lo = new_cluster & 0xFFFF;
                entries[i].file_size = 0;

                if (fat32_write_cluster(fs, cluster, cluster_buf) != 0)
                {
                    free(cluster_buf);
                    fat32_free_cluster(fs, new_cluster);
                    return -1;
                }

                free(cluster_buf);
                return 0;
            }
        }

        uint32_t next_cluster = fat32_get_next_cluster(fs, cluster);
        if (next_cluster >= FAT32_EOC_MIN)
        {
            uint32_t new_dir_cluster = fat32_alloc_cluster(fs);
            if (new_dir_cluster == 0)
            {
                free(cluster_buf);
                fat32_free_cluster(fs, new_cluster);
                return -1;
            }

            fat32_set_fat_entry(fs, cluster, new_dir_cluster);
            cluster = new_dir_cluster;

            memset(cluster_buf, 0, fs->cluster_size);
            fat32_write_cluster(fs, cluster, cluster_buf);
        }
        else
        {
            cluster = next_cluster;
        }
    }

    free(cluster_buf);
    fat32_free_cluster(fs, new_cluster);
    return -1;
}

int fat32_mkdir(vfs_node_t *dir, const char *name, mode_t mode)
{
    return fat32_create(dir, name, mode | S_IFDIR);
}

int fat32_rmdir(vfs_node_t *dir, const char *name)
{
    if (!dir || !name || !vfs_is_directory(dir))
    {
        return -1;
    }

    fat32_fs_t *fs = (fat32_fs_t *)dir->sb->fs_data;
    fat32_inode_t *dir_inode = (fat32_inode_t *)dir->fs_data;
    if (!fs || !dir_inode)
    {
        return -1;
    }

    /* First look up the target so we can verify it's empty */
    vfs_node_t *target = fat32_lookup(dir, name);
    if (!target || !vfs_is_directory(target))
    {
        if (target)
            free(target);
        return -1;
    }

    fat32_inode_t *target_inode = (fat32_inode_t *)target->fs_data;
    uint32_t target_cluster = target_inode->first_cluster;

    /* Check the target directory is empty (no entries other than
     * deleted/LFN/volume-id markers) */
    uint8_t *check_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!check_buf)
    {
        free(target->fs_data);
        free(target);
        return -1;
    }

    uint32_t scan = target_cluster;
    while (scan >= 2 && scan < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, scan, check_buf) != 0)
            break;

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)check_buf;
        int num = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num; i++)
        {
            if (entries[i].name[0] == 0x00)
                goto scan_done; /* end of entries */
            if ((uint8_t)entries[i].name[0] == 0xE5)
                continue; /* deleted */
            if (entries[i].attr == FAT32_ATTR_LFN)
                continue; /* LFN part */
            if (entries[i].attr & FAT32_ATTR_VOLUME_ID)
                continue; /* volume label */
            /* Found a live entry - directory is not empty */
            free(check_buf);
            free(target->fs_data);
            free(target);
            return -1;
        }
        scan = fat32_get_next_cluster(fs, scan);
    }
scan_done:
    free(check_buf);

    /* Directory is empty - free its cluster chain */
    fat32_free_chain(fs, target_cluster);

    /* Now mark the entry in the parent as deleted (fall through to
     * the shared deletion logic below) */
    free(target->fs_data);
    free(target);

    /* Walk parent dir and mark the matching entry as 0xE5 */
    uint32_t cluster = dir_inode->first_cluster;
    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
        return -1;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return -1;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int num = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num; i++)
        {
            if (entries[i].name[0] == 0x00)
            {
                free(cluster_buf);
                return -1;
            }
            if ((uint8_t)entries[i].name[0] == 0xE5)
                continue;
            if (entries[i].attr == FAT32_ATTR_LFN)
                continue;

            char entry_name[256];
            fat32_83_to_name(entries[i].name, entry_name);

            if (strcmp(entry_name, name) == 0)
            {
                entries[i].name[0] = (char)0xE5;
                fat32_write_cluster(fs, cluster, cluster_buf);
                free(cluster_buf);
                return 0;
            }
        }
        cluster = fat32_get_next_cluster(fs, cluster);
    }

    free(cluster_buf);
    return -1;
}

int fat32_unlink(vfs_node_t *dir, const char *name)
{
    if (!dir || !name || !vfs_is_directory(dir))
    {
        return -1;
    }

    fat32_fs_t *fs = (fat32_fs_t *)dir->sb->fs_data;
    fat32_inode_t *dir_inode = (fat32_inode_t *)dir->fs_data;
    if (!fs || !dir_inode)
    {
        return -1;
    }

    uint32_t cluster = dir_inode->first_cluster;
    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        return -1;
    }

    while (cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return -1;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int num = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num; i++)
        {
            if (entries[i].name[0] == 0x00)
            {
                free(cluster_buf);
                return -1; /* hit end of dir, name not found */
            }

            if ((uint8_t)entries[i].name[0] == 0xE5)
                continue;
            if (entries[i].attr == FAT32_ATTR_LFN)
                continue;
            if (entries[i].attr & FAT32_ATTR_DIRECTORY)
                continue; /* unlink is for files only */

            char entry_name[256];
            fat32_83_to_name(entries[i].name, entry_name);

            if (strcmp(entry_name, name) == 0)
            {
                /* Free the file's data clusters */
                uint32_t first = ((uint32_t)entries[i].first_cluster_hi << 16) |
                                 entries[i].first_cluster_lo;
                if (first >= 2)
                {
                    fat32_free_chain(fs, first);
                }

                /* Mark directory entry as deleted */
                entries[i].name[0] = (char)0xE5;

                if (fat32_write_cluster(fs, cluster, cluster_buf) != 0)
                {
                    free(cluster_buf);
                    return -1;
                }

                free(cluster_buf);
                return 0;
            }
        }
        cluster = fat32_get_next_cluster(fs, cluster);
    }

    free(cluster_buf);
    return -1; /* name not found */
}

int fat32_open(vfs_node_t *node, vfs_file_t *file)
{
    return 0;
}

int fat32_close(vfs_node_t *node, vfs_file_t *file)
{
    return 0;
}

ssize_t fat32_read(vfs_file_t *file, void *buf, size_t count, off_t offset)
{
    if (!file || !buf)
    {
        return -1;
    }

    vfs_node_t *node = file->node;
    fat32_fs_t *fs = (fat32_fs_t *)node->sb->fs_data;
    fat32_inode_t *inode = (fat32_inode_t *)node->fs_data;

    if (!fs || !inode)
    {
        return -1;
    }

    if (offset >= (off_t)node->size)
    {
        return 0;
    }

    if (offset + count > node->size)
    {
        count = node->size - offset;
    }

    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        return -1;
    }

    size_t bytes_read = 0;
    uint32_t cluster = inode->first_cluster;
    uint32_t cluster_offset = offset / fs->cluster_size;
    uint32_t byte_offset = offset % fs->cluster_size;

    for (uint32_t i = 0; i < cluster_offset && cluster >= 2 && cluster < FAT32_EOC_MIN; i++)
    {
        cluster = fat32_get_next_cluster(fs, cluster);
    }

    while (bytes_read < count && cluster >= 2 && cluster < FAT32_EOC_MIN)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return bytes_read > 0 ? bytes_read : -1;
        }

        size_t to_read = fs->cluster_size - byte_offset;
        if (to_read > count - bytes_read)
        {
            to_read = count - bytes_read;
        }

        memcpy((uint8_t *)buf + bytes_read, cluster_buf + byte_offset, to_read);
        bytes_read += to_read;
        byte_offset = 0;

        cluster = fat32_get_next_cluster(fs, cluster);
    }

    free(cluster_buf);
    return bytes_read;
}

ssize_t fat32_write(vfs_file_t *file, const void *buf, size_t count, off_t offset)
{
    if (!file || !buf)
    {
        return -1;
    }

    vfs_node_t *node = file->node;
    fat32_fs_t *fs = (fat32_fs_t *)node->sb->fs_data;
    fat32_inode_t *inode = (fat32_inode_t *)node->fs_data;

    if (!fs || !inode)
    {
        return -1;
    }

    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        return -1;
    }

    size_t bytes_written = 0;
    uint32_t cluster = inode->first_cluster;
    uint32_t cluster_offset = offset / fs->cluster_size;
    uint32_t byte_offset = offset % fs->cluster_size;

    uint32_t prev_cluster = 0;
    for (uint32_t i = 0; i < cluster_offset; i++)
    {
        if (cluster >= FAT32_EOC_MIN)
        {
            uint32_t new_cluster = fat32_alloc_cluster(fs);
            if (new_cluster == 0)
            {
                free(cluster_buf);
                return bytes_written > 0 ? bytes_written : -1;
            }

            if (prev_cluster != 0)
            {
                fat32_set_fat_entry(fs, prev_cluster, new_cluster);
            }
            else
            {
                inode->first_cluster = new_cluster;
            }

            cluster = new_cluster;
        }

        prev_cluster = cluster;
        if (cluster >= 2 && cluster < FAT32_EOC_MIN)
        {
            cluster = fat32_get_next_cluster(fs, cluster);
        }
    }

    while (bytes_written < count)
    {
        if (cluster >= FAT32_EOC_MIN)
        {
            uint32_t new_cluster = fat32_alloc_cluster(fs);
            if (new_cluster == 0)
            {
                free(cluster_buf);
                break;
            }

            if (prev_cluster != 0)
            {
                fat32_set_fat_entry(fs, prev_cluster, new_cluster);
            }
            else
            {
                inode->first_cluster = new_cluster;
            }

            cluster = new_cluster;
        }

        if (byte_offset != 0 || count - bytes_written < fs->cluster_size)
        {
            if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
            {
                memset(cluster_buf, 0, fs->cluster_size);
            }
        }

        size_t to_write = fs->cluster_size - byte_offset;
        if (to_write > count - bytes_written)
        {
            to_write = count - bytes_written;
        }

        memcpy(cluster_buf + byte_offset, (const uint8_t *)buf + bytes_written, to_write);

        if (fat32_write_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return bytes_written > 0 ? bytes_written : -1;
        }

        bytes_written += to_write;
        byte_offset = 0;
        prev_cluster = cluster;
        cluster = fat32_get_next_cluster(fs, cluster);
    }

    if (offset + bytes_written > (off_t)node->size)
    {
        node->size = offset + bytes_written;
    }

    free(cluster_buf);
    return bytes_written;
}

int fat32_readdir(vfs_file_t *file, struct dirent *dirent, size_t count)
{
    if (!file || !dirent || !vfs_is_directory(file->node))
    {
        return -1;
    }

    vfs_node_t *dir = file->node;
    fat32_fs_t *fs = (fat32_fs_t *)dir->sb->fs_data;
    fat32_inode_t *dir_inode = (fat32_inode_t *)dir->fs_data;

    if (!fs || !dir_inode)
    {
        return -1;
    }

    uint32_t cluster = dir_inode->first_cluster;
    uint8_t *cluster_buf = (uint8_t *)malloc(fs->cluster_size);
    if (!cluster_buf)
    {
        return -1;
    }

    int entry_index = (int)(file->offset / sizeof(fat32_dir_entry_t));
    int current_index = 0;
    int entries_read = 0;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN && entries_read < (int)count)
    {
        if (fat32_read_cluster(fs, cluster, cluster_buf) != 0)
        {
            free(cluster_buf);
            return entries_read;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int num_entries = fs->cluster_size / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < num_entries && entries_read < (int)count; i++)
        {
            if (entries[i].name[0] == 0x00)
            {
                free(cluster_buf);
                return entries_read;
            }

            if ((uint8_t)entries[i].name[0] == 0xE5)
            {
                current_index++;
                continue;
            }

            if (entries[i].attr == FAT32_ATTR_LFN)
            {
                current_index++;
                continue;
            }

            if (entries[i].attr & FAT32_ATTR_VOLUME_ID)
            {
                current_index++;
                continue;
            }

            if (current_index < entry_index)
            {
                current_index++;
                continue;
            }

            uint32_t first_cluster = ((uint32_t)entries[i].first_cluster_hi << 16) |
                                     entries[i].first_cluster_lo;

            dirent[entries_read].d_ino = first_cluster;
            dirent[entries_read].d_off = current_index + 1;
            dirent[entries_read].d_reclen = sizeof(struct dirent);
            dirent[entries_read].d_type = (entries[i].attr & FAT32_ATTR_DIRECTORY) ? VFS_DIRECTORY : VFS_FILE;

            fat32_83_to_name(entries[i].name, dirent[entries_read].d_name);

            entries_read++;
            current_index++;
            file->offset = current_index * sizeof(fat32_dir_entry_t);
        }

        cluster = fat32_get_next_cluster(fs, cluster);
    }

    free(cluster_buf);
    return entries_read;
}

vfs_superblock_t *fat32_mount(const char *dev, uint32_t flags)
{
    (void)flags;

    if (!dev)
    {
        printf("[FAT32] Error: NULL device name\n");
        return NULL;
    }

    struct block_device *blkdev = blkdev_find(dev);
    if (!blkdev)
    {
        printf("[FAT32] Block device '%s' not found\n", dev);
        return NULL;
    }

    fat32_fs_t *fs = (fat32_fs_t *)malloc(sizeof(fat32_fs_t));
    if (!fs)
    {
        printf("[FAT32] Failed to allocate filesystem structure\n");
        return NULL;
    }

    memset(fs, 0, sizeof(fat32_fs_t));
    fs->dev = blkdev;

    /* Read boot sector into a full 512-byte buffer first.
     * CRITICAL: fat32_boot_sector_t is only 90 bytes (packed).
     * Writing 512 raw bytes directly into &fs->boot overflows
     * by 422 bytes, trashing the rest of fat32_fs_t and the
     * next heap block's magic number. That is what was causing
     * the heap corruption detected on every subsequent malloc. */
    {
        uint8_t boot_buf[512];
        if (blkdev_read(blkdev, 0, 1, boot_buf) != 0)
        {
            printf("[FAT32] Failed to read boot sector\n");
            free(fs);
            return NULL;
        }
        memcpy(&fs->boot, boot_buf, sizeof(fs->boot));
    }

    if (fs->boot.bytes_per_sector != 512)
    {
        printf("[FAT32] Unsupported sector size: %d\n", fs->boot.bytes_per_sector);
        free(fs);
        return NULL;
    }

    if (fs->boot.fat_size_16 != 0 || fs->boot.root_entry_count != 0)
    {
        printf("[FAT32] Not a FAT32 filesystem\n");
        free(fs);
        return NULL;
    }

    fs->fat_offset = fs->boot.reserved_sectors;
    fs->data_offset = fs->fat_offset + (fs->boot.num_fats * fs->boot.fat_size_32);
    fs->root_cluster = fs->boot.root_cluster;

    uint32_t total_sectors = fs->boot.total_sectors_32;
    uint32_t data_sectors = total_sectors - fs->data_offset;
    fs->total_clusters = data_sectors / fs->boot.sectors_per_cluster;
    fs->cluster_size = fs->boot.bytes_per_sector * fs->boot.sectors_per_cluster;

    // NO FAT CACHING - too large for heap!
    fs->fat_cache = NULL;
    fs->fat_cache_size = 0;
    fs->fat_dirty = 0;

    vfs_superblock_t *sb = (vfs_superblock_t *)malloc(sizeof(vfs_superblock_t));
    if (!sb)
    {
        free(fs);
        return NULL;
    }

    memset(sb, 0, sizeof(vfs_superblock_t));
    sb->fs_type = "fat32";
    sb->blocksize = fs->cluster_size;
    sb->total_blocks = fs->total_clusters;
    sb->s_ops = &fat32_sb_ops;
    sb->fs_data = fs;

    vfs_node_t *root = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    if (!root)
    {
        free(fs);
        free(sb);
        return NULL;
    }

    memset(root, 0, sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->inode = fs->root_cluster;
    root->mode = S_IFDIR | 0755;
    root->size = 0;
    root->nlink = 1;
    root->sb = sb;
    root->fops = &fat32_file_ops;
    root->iops = &fat32_inode_ops;

    fat32_inode_t *root_inode = (fat32_inode_t *)malloc(sizeof(fat32_inode_t));
    if (!root_inode)
    {
        free(root);
        free(fs);
        free(sb);
        return NULL;
    }

    root_inode->first_cluster = fs->root_cluster;
    root_inode->dir_cluster = 0;
    root_inode->dir_offset = 0;
    root->fs_data = root_inode;

    sb->root = root;

    return sb;
}

void fat32_unmount(vfs_superblock_t *sb)
{
    if (!sb)
    {
        return;
    }

    fat32_fs_t *fs = (fat32_fs_t *)sb->fs_data;
    if (fs)
    {
        if (fs->fat_cache)
        {
            free(fs->fat_cache);
        }
        free(fs);
    }

    if (sb->root)
    {
        if (sb->root->fs_data)
        {
            free(sb->root->fs_data);
        }
        free(sb->root);
    }

    free(sb);
}

int fat32_init(void)
{
    spin_lock_init(&fat32_lock, "fat32");

    fat32_file_ops.open = fat32_open;
    fat32_file_ops.close = fat32_close;
    fat32_file_ops.read = fat32_read;
    fat32_file_ops.write = fat32_write;
    fat32_file_ops.readdir = fat32_readdir;
    fat32_file_ops.ioctl = NULL;

    fat32_inode_ops.lookup = fat32_lookup;
    fat32_inode_ops.create = fat32_create;
    fat32_inode_ops.mkdir = fat32_mkdir;
    fat32_inode_ops.rmdir = fat32_rmdir;
    fat32_inode_ops.unlink = fat32_unlink;
    fat32_inode_ops.symlink = NULL;
    fat32_inode_ops.readlink = NULL;

    fat32_sb_ops.alloc_inode = NULL;
    fat32_sb_ops.destroy_inode = NULL;
    fat32_sb_ops.write_inode = NULL;
    fat32_sb_ops.sync_fs = NULL;

    fat32_filesystem.name = "fat32";
    fat32_filesystem.mount = fat32_mount;
    fat32_filesystem.unmount = fat32_unmount;
    fat32_filesystem.next = NULL;

    if (vfs_register_filesystem(&fat32_filesystem) != 0)
    {
        printf("[FAT32] Failed to register filesystem\n");
        return -1;
    }

    return 0;
}
