/*
 * Copyright (c) 2026 Trollycat
 * Virtual Filesystem (VFS) Layer Implementation
 */

#include <thuban/vfs.h>
#include <thuban/string.h>
#include <thuban/stdio.h>
#include <thuban/heap.h>
#include <thuban/spinlock.h>

static vfs_mount_t *mount_list = NULL;
static vfs_filesystem_t *fs_list = NULL;
static vfs_file_t *fd_table[VFS_MAX_OPEN_FILES];
static vfs_node_t *current_working_dir = NULL;
static spinlock_t vfs_lock;

void vfs_init(void)
{
    spin_lock_init(&vfs_lock, "vfs");

    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++)
    {
        fd_table[i] = NULL;
    }

    mount_list = NULL;
    fs_list = NULL;
    current_working_dir = NULL;

    printf("[VFS] Virtual filesystem initialized\n");
}

int vfs_register_filesystem(vfs_filesystem_t *fs)
{
    if (!fs || !fs->name || !fs->mount)
    {
        return -1;
    }

    spin_lock(&vfs_lock);

    vfs_filesystem_t *existing = fs_list;
    while (existing)
    {
        if (strcmp(existing->name, fs->name) == 0)
        {
            spin_unlock(&vfs_lock);
            return -1;
        }
        existing = existing->next;
    }

    fs->next = fs_list;
    fs_list = fs;

    spin_unlock(&vfs_lock);

    printf("[VFS] Registered filesystem: %s\n", fs->name);
    return 0;
}

int vfs_unregister_filesystem(const char *name)
{
    spin_lock(&vfs_lock);

    vfs_filesystem_t **curr = &fs_list;
    while (*curr)
    {
        if (strcmp((*curr)->name, name) == 0)
        {
            *curr = (*curr)->next;
            spin_unlock(&vfs_lock);
            printf("[VFS] Unregistered filesystem: %s\n", name);
            return 0;
        }
        curr = &(*curr)->next;
    }

    spin_unlock(&vfs_lock);
    return -1;
}

static vfs_filesystem_t *vfs_find_filesystem(const char *name)
{
    vfs_filesystem_t *fs = fs_list;
    while (fs)
    {
        if (strcmp(fs->name, name) == 0)
        {
            return fs;
        }
        fs = fs->next;
    }
    return NULL;
}

int vfs_mount(const char *dev, const char *mountpoint, const char *fstype, uint32_t flags)
{
    if (!dev || !mountpoint || !fstype)
    {
        return -1;
    }

    vfs_filesystem_t *fs = vfs_find_filesystem(fstype);
    if (!fs)
    {
        printf("[VFS] Unknown filesystem type: %s\n", fstype);
        return -1;
    }

    vfs_superblock_t *sb = fs->mount(dev, flags);
    if (!sb)
    {
        printf("[VFS] Failed to mount %s on %s\n", dev, mountpoint);
        return -1;
    }

    vfs_mount_t *mount = (vfs_mount_t *)malloc(sizeof(vfs_mount_t));
    if (!mount)
    {
        if (fs->unmount)
        {
            fs->unmount(sb);
        }
        return -1;
    }

    mount->mountpoint = (char *)malloc(strlen(mountpoint) + 1);
    if (!mount->mountpoint)
    {
        free(mount);
        if (fs->unmount)
        {
            fs->unmount(sb);
        }
        return -1;
    }

    strcpy(mount->mountpoint, mountpoint);
    mount->sb = sb;
    mount->root = sb->root;
    sb->mount = mount;

    spin_lock(&vfs_lock);
    mount->next = mount_list;
    mount_list = mount;
    spin_unlock(&vfs_lock);

    if (strcmp(mountpoint, "/") == 0 && current_working_dir == NULL)
    {
        current_working_dir = sb->root;
    }

    printf("[VFS] Mounted %s (%s) on %s\n", dev, fstype, mountpoint);
    return 0;
}

int vfs_unmount(const char *mountpoint)
{
    spin_lock(&vfs_lock);

    vfs_mount_t **curr = &mount_list;
    while (*curr)
    {
        if (strcmp((*curr)->mountpoint, mountpoint) == 0)
        {
            vfs_mount_t *mount = *curr;
            *curr = mount->next;

            spin_unlock(&vfs_lock);

            vfs_filesystem_t *fs = vfs_find_filesystem(mount->sb->fs_type);
            if (fs && fs->unmount)
            {
                fs->unmount(mount->sb);
            }

            free(mount->mountpoint);
            free(mount);

            printf("[VFS] Unmounted %s\n", mountpoint);
            return 0;
        }
        curr = &(*curr)->next;
    }

    spin_unlock(&vfs_lock);
    return -1;
}

static vfs_mount_t *vfs_find_mount(const char *path)
{
    vfs_mount_t *best_mount = NULL;
    size_t best_len = 0;

    vfs_mount_t *mount = mount_list;
    while (mount)
    {
        size_t len = strlen(mount->mountpoint);
        if (strncmp(path, mount->mountpoint, len) == 0)
        {
            if (len > best_len)
            {
                best_mount = mount;
                best_len = len;
            }
        }
        mount = mount->next;
    }

    return best_mount;
}

vfs_node_t *vfs_resolve_path(const char *path)
{
    if (!path || !current_working_dir)
    {
        return NULL;
    }

    if (path[0] == '/')
    {
        vfs_mount_t *mount = vfs_find_mount(path);
        if (!mount)
        {
            return NULL;
        }

        const char *relative_path = path + strlen(mount->mountpoint);
        if (*relative_path == '\0')
        {
            return mount->root;
        }
        if (*relative_path == '/')
        {
            relative_path++;
        }

        return vfs_resolve_path_from(mount->root, relative_path);
    }

    return vfs_resolve_path_from(current_working_dir, path);
}

vfs_node_t *vfs_resolve_path_from(vfs_node_t *start, const char *path)
{
    if (!start || !path)
    {
        return NULL;
    }

    if (*path == '\0')
    {
        return start;
    }

    char path_copy[VFS_MAX_PATH];
    strncpy(path_copy, path, VFS_MAX_PATH - 1);
    path_copy[VFS_MAX_PATH - 1] = '\0';

    vfs_node_t *current = start;
    char *token = path_copy;
    char *next;

    while (token && *token)
    {
        next = token;
        while (*next && *next != '/')
        {
            next++;
        }

        if (*next == '/')
        {
            *next = '\0';
            next++;
        }
        else
        {
            next = NULL;
        }

        if (*token == '\0')
        {
            token = next;
            continue;
        }

        if (strcmp(token, ".") == 0)
        {
            token = next;
            continue;
        }

        if (strcmp(token, "..") == 0)
        {
            if (current->parent)
            {
                current = current->parent;
            }
            token = next;
            continue;
        }

        if (!vfs_is_directory(current))
        {
            return NULL;
        }

        if (!current->iops || !current->iops->lookup)
        {
            return NULL;
        }

        vfs_node_t *child = current->iops->lookup(current, token);
        if (!child)
        {
            return NULL;
        }

        current = child;
        token = next;
    }

    return current;
}

int vfs_alloc_fd(vfs_file_t *file)
{
    spin_lock(&vfs_lock);

    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++)
    {
        if (fd_table[i] == NULL)
        {
            fd_table[i] = file;
            file->refcount++;
            spin_unlock(&vfs_lock);
            return i;
        }
    }

    spin_unlock(&vfs_lock);
    return -1;
}

void vfs_free_fd(int fd)
{
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES)
    {
        return;
    }

    spin_lock(&vfs_lock);

    if (fd_table[fd])
    {
        fd_table[fd]->refcount--;
        if (fd_table[fd]->refcount == 0)
        {
            free(fd_table[fd]);
        }
        fd_table[fd] = NULL;
    }

    spin_unlock(&vfs_lock);
}

vfs_file_t *vfs_get_file(int fd)
{
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES)
    {
        return NULL;
    }
    return fd_table[fd];
}

int vfs_open(const char *path, int flags, mode_t mode)
{
    if (!path)
    {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);

    if (!node && (flags & O_CREAT))
    {
        char dir_path[VFS_MAX_PATH];
        strncpy(dir_path, path, VFS_MAX_PATH - 1);
        dir_path[VFS_MAX_PATH - 1] = '\0';

        char *last_slash = strrchr(dir_path, '/');
        if (!last_slash)
        {
            return -1;
        }

        *last_slash = '\0';
        const char *filename = last_slash + 1;

        vfs_node_t *dir = vfs_resolve_path(dir_path);
        if (!dir || !vfs_is_directory(dir))
        {
            return -1;
        }

        if (!dir->iops || !dir->iops->create)
        {
            return -1;
        }

        if (dir->iops->create(dir, filename, mode) != 0)
        {
            return -1;
        }

        node = vfs_resolve_path(path);
        if (!node)
        {
            return -1;
        }
    }

    if (!node)
    {
        return -1;
    }

    if (vfs_is_directory(node) && !(flags & O_DIRECTORY))
    {
        if (!(flags & O_RDONLY))
        {
            return -1;
        }
    }

    vfs_file_t *file = (vfs_file_t *)malloc(sizeof(vfs_file_t));
    if (!file)
    {
        return -1;
    }

    file->node = node;
    file->offset = 0;
    file->flags = flags;
    file->mode = mode;
    file->refcount = 0;

    if (flags & O_TRUNC)
    {
        node->size = 0;
    }

    if (node->fops && node->fops->open)
    {
        if (node->fops->open(node, file) != 0)
        {
            free(file);
            return -1;
        }
    }

    int fd = vfs_alloc_fd(file);
    if (fd < 0)
    {
        if (node->fops && node->fops->close)
        {
            node->fops->close(node, file);
        }
        free(file);
        return -1;
    }

    return fd;
}

int vfs_close(int fd)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file)
    {
        return -1;
    }

    if (file->node->fops && file->node->fops->close)
    {
        file->node->fops->close(file->node, file);
    }

    vfs_free_fd(fd);
    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t count)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file || !buf)
    {
        return -1;
    }

    if (!file->node->fops || !file->node->fops->read)
    {
        return -1;
    }

    ssize_t bytes_read = file->node->fops->read(file, buf, count, file->offset);
    if (bytes_read > 0)
    {
        file->offset += bytes_read;
    }

    return bytes_read;
}

ssize_t vfs_write(int fd, const void *buf, size_t count)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file || !buf)
    {
        return -1;
    }

    if (!file->node->fops || !file->node->fops->write)
    {
        return -1;
    }

    if ((file->flags & O_ACCMODE) == O_RDONLY)
    {
        return -1;
    }

    if (file->flags & O_APPEND)
    {
        file->offset = file->node->size;
    }

    ssize_t bytes_written = file->node->fops->write(file, buf, count, file->offset);
    if (bytes_written > 0)
    {
        file->offset += bytes_written;
    }

    return bytes_written;
}

off_t vfs_lseek(int fd, off_t offset, int whence)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file)
    {
        return -1;
    }

    off_t new_offset;

    switch (whence)
    {
    case SEEK_SET:
        new_offset = offset;
        break;
    case SEEK_CUR:
        new_offset = file->offset + offset;
        break;
    case SEEK_END:
        new_offset = file->node->size + offset;
        break;
    default:
        return -1;
    }

    if (new_offset < 0)
    {
        return -1;
    }

    file->offset = new_offset;
    return new_offset;
}

int vfs_stat(const char *path, struct stat *buf)
{
    if (!path || !buf)
    {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node)
    {
        return -1;
    }

    buf->st_dev = node->dev;
    buf->st_ino = node->inode;
    buf->st_mode = node->mode;
    buf->st_nlink = node->nlink;
    buf->st_uid = node->uid;
    buf->st_gid = node->gid;
    buf->st_rdev = 0;
    buf->st_size = node->size;
    buf->st_atime = node->atime;
    buf->st_mtime = node->mtime;
    buf->st_ctime = node->ctime;
    buf->st_blksize = node->sb ? node->sb->blocksize : 512;
    buf->st_blocks = (node->size + 511) / 512;

    return 0;
}

int vfs_fstat(int fd, struct stat *buf)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file || !buf)
    {
        return -1;
    }

    vfs_node_t *node = file->node;

    buf->st_dev = node->dev;
    buf->st_ino = node->inode;
    buf->st_mode = node->mode;
    buf->st_nlink = node->nlink;
    buf->st_uid = node->uid;
    buf->st_gid = node->gid;
    buf->st_rdev = 0;
    buf->st_size = node->size;
    buf->st_atime = node->atime;
    buf->st_mtime = node->mtime;
    buf->st_ctime = node->ctime;
    buf->st_blksize = node->sb ? node->sb->blocksize : 512;
    buf->st_blocks = (node->size + 511) / 512;

    return 0;
}

int vfs_readdir(int fd, struct dirent *dirent, size_t count)
{
    vfs_file_t *file = vfs_get_file(fd);
    if (!file || !dirent)
    {
        return -1;
    }

    if (!vfs_is_directory(file->node))
    {
        return -1;
    }

    if (!file->node->fops || !file->node->fops->readdir)
    {
        return -1;
    }

    return file->node->fops->readdir(file, dirent, count);
}

int vfs_mkdir(const char *path, mode_t mode)
{
    if (!path)
    {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (node)
    {
        return -1;
    }

    char dir_path[VFS_MAX_PATH];
    strncpy(dir_path, path, VFS_MAX_PATH - 1);
    dir_path[VFS_MAX_PATH - 1] = '\0';

    char *last_slash = strrchr(dir_path, '/');
    if (!last_slash)
    {
        return -1;
    }

    *last_slash = '\0';
    const char *dirname = last_slash + 1;

    vfs_node_t *parent = vfs_resolve_path(dir_path);
    if (!parent || !vfs_is_directory(parent))
    {
        return -1;
    }

    if (!parent->iops || !parent->iops->mkdir)
    {
        return -1;
    }

    return parent->iops->mkdir(parent, dirname, mode | S_IFDIR);
}

int vfs_rmdir(const char *path)
{
    if (!path)
    {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node || !vfs_is_directory(node))
    {
        return -1;
    }

    if (!node->parent)
    {
        return -1;
    }

    if (!node->parent->iops || !node->parent->iops->rmdir)
    {
        return -1;
    }

    return node->parent->iops->rmdir(node->parent, node->name);
}

int vfs_unlink(const char *path)
{
    if (!path)
    {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node || vfs_is_directory(node))
    {
        return -1;
    }

    if (!node->parent)
    {
        return -1;
    }

    if (!node->parent->iops || !node->parent->iops->unlink)
    {
        return -1;
    }

    return node->parent->iops->unlink(node->parent, node->name);
}

int vfs_is_directory(vfs_node_t *node)
{
    return node && ((node->mode & S_IFMT) == S_IFDIR);
}

int vfs_is_file(vfs_node_t *node)
{
    return node && ((node->mode & S_IFMT) == S_IFREG);
}

vfs_node_t *vfs_get_cwd(void)
{
    return current_working_dir;
}

int vfs_set_cwd(vfs_node_t *node)
{
    if (!node || !vfs_is_directory(node))
    {
        return -1;
    }
    current_working_dir = node;
    return 0;
}

char *vfs_basename(const char *path)
{
    if (!path)
    {
        return NULL;
    }

    const char *last_slash = strrchr(path, '/');
    if (last_slash)
    {
        return (char *)(last_slash + 1);
    }
    return (char *)path;
}

char *vfs_dirname(const char *path)
{
    static char dirname_buf[VFS_MAX_PATH];

    if (!path)
    {
        return NULL;
    }

    strncpy(dirname_buf, path, VFS_MAX_PATH - 1);
    dirname_buf[VFS_MAX_PATH - 1] = '\0';

    char *last_slash = strrchr(dirname_buf, '/');
    if (last_slash)
    {
        *last_slash = '\0';
        return dirname_buf;
    }

    return ".";
}