/*
 * Copyright (c) 2026 Trollycat
 * Virtual file system.
 */

#ifndef THUBAN_VFS_H
#define THUBAN_VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHARDEVICE 0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE 0x05
#define VFS_SYMLINK 0x06
#define VFS_MOUNTPOINT 0x08

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_ACCMODE 0x0003
#define O_CREAT 0x0040
#define O_EXCL 0x0080
#define O_TRUNC 0x0200
#define O_APPEND 0x0400
#define O_DIRECTORY 0x10000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define VFS_MAX_PATH 4096
#define VFS_MAX_NAME 256
#define VFS_MAX_OPEN_FILES 256

#define S_IFMT 0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000

#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define EPERM 1
#define ENOENT 2
#define EIO 5
#define EACCES 13
#define EEXIST 17
#define ENOTDIR 20
#define EISDIR 21

typedef uint32_t mode_t;
typedef int64_t off_t;
typedef uint32_t ino_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int64_t time_t;
typedef long ssize_t;

struct vfs_node;
struct vfs_file;
struct vfs_superblock;
struct vfs_mount;

struct dirent
{
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[VFS_MAX_NAME];
};

struct stat
{
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    uint32_t st_blksize;
    uint64_t st_blocks;
};

typedef struct vfs_file_operations
{
    int (*open)(struct vfs_node *node, struct vfs_file *file);
    int (*close)(struct vfs_node *node, struct vfs_file *file);
    ssize_t (*read)(struct vfs_file *file, void *buf, size_t count, off_t offset);
    ssize_t (*write)(struct vfs_file *file, const void *buf, size_t count, off_t offset);
    int (*readdir)(struct vfs_file *file, struct dirent *dirent, size_t count);
    int (*ioctl)(struct vfs_file *file, unsigned long request, void *arg);
} vfs_file_operations_t;

typedef struct vfs_inode_operations
{
    struct vfs_node *(*lookup)(struct vfs_node *dir, const char *name);
    int (*create)(struct vfs_node *dir, const char *name, mode_t mode);
    int (*mkdir)(struct vfs_node *dir, const char *name, mode_t mode);
    int (*rmdir)(struct vfs_node *dir, const char *name);
    int (*unlink)(struct vfs_node *dir, const char *name);
} vfs_inode_operations_t;

typedef struct vfs_superblock_operations
{
    struct vfs_node *(*alloc_inode)(struct vfs_superblock *sb);
    void (*destroy_inode)(struct vfs_node *node);
    int (*write_inode)(struct vfs_node *node);
    int (*sync_fs)(struct vfs_superblock *sb);
} vfs_superblock_operations_t;

typedef struct vfs_node
{
    char name[VFS_MAX_NAME];
    ino_t inode;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    size_t size;
    nlink_t nlink;
    time_t atime;
    time_t mtime;
    time_t ctime;
    dev_t dev;
    uint32_t flags;
    uint32_t refcount;
    vfs_file_operations_t *fops;
    vfs_inode_operations_t *iops;
    void *fs_data;
    struct vfs_superblock *sb;
    struct vfs_node *parent;
} vfs_node_t;

typedef struct vfs_superblock
{
    dev_t dev;
    const char *fs_type;
    uint32_t blocksize;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint32_t flags;
    struct vfs_node *root;
    vfs_superblock_operations_t *s_ops;
    void *fs_data;
    struct vfs_mount *mount;
} vfs_superblock_t;

typedef struct vfs_file
{
    struct vfs_node *node;
    off_t offset;
    uint32_t flags;
    mode_t mode;
    uint32_t refcount;
} vfs_file_t;

typedef struct vfs_mount
{
    char *mountpoint;
    struct vfs_superblock *sb;
    struct vfs_node *root;
    struct vfs_mount *next;
} vfs_mount_t;

typedef struct vfs_filesystem
{
    const char *name;
    struct vfs_superblock *(*mount)(const char *dev, uint32_t flags);
    void (*unmount)(struct vfs_superblock *sb);
    struct vfs_filesystem *next;
} vfs_filesystem_t;

void vfs_init(void);
int vfs_register_filesystem(vfs_filesystem_t *fs);
int vfs_mount(const char *dev, const char *mountpoint, const char *fstype, uint32_t flags);
vfs_node_t *vfs_resolve_path(const char *path);
vfs_node_t *vfs_resolve_path_from(vfs_node_t *start, const char *path);
int vfs_open(const char *path, int flags, mode_t mode);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void *buf, size_t count);
ssize_t vfs_write(int fd, const void *buf, size_t count);
off_t vfs_lseek(int fd, off_t offset, int whence);
int vfs_stat(const char *path, struct stat *buf);
int vfs_fstat(int fd, struct stat *buf);
int vfs_readdir(int fd, struct dirent *dirent, size_t count);
int vfs_mkdir(const char *path, mode_t mode);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_is_directory(vfs_node_t *node);
char *vfs_basename(const char *path);
vfs_node_t *vfs_get_cwd(void);
int vfs_set_cwd(vfs_node_t *node);

#endif
