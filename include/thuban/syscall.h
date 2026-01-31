/*
 * Copyright (c) 2026 Trollycat
 * System call interface for Thuban
 */

#ifndef THUBAN_SYSCALL_H
#define THUBAN_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

/* Define ssize_t if not already defined */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef int64_t off_t;
#endif

/* Forward declarations for VFS types */
struct stat;
struct dirent;

/* System call numbers */
#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_OPEN 3
#define SYS_CLOSE 4
#define SYS_GETPID 5
#define SYS_FORK 6
#define SYS_EXEC 7
#define SYS_WAIT 8
#define SYS_SBRK 9
#define SYS_SLEEP 10
#define SYS_YIELD 11
#define SYS_GETTIME 12
#define SYS_LSEEK 13
#define SYS_STAT 14
#define SYS_FSTAT 15
#define SYS_MKDIR 16
#define SYS_RMDIR 17
#define SYS_GETDENTS 18
#define SYS_UNLINK 19

#define SYSCALL_MAX 256

/* MSR (Model Specific Register) addresses for SYSCALL/SYSRET */
#define MSR_STAR 0xC0000081   /* Segment selectors for syscall */
#define MSR_LSTAR 0xC0000082  /* Syscall entry point (RIP) */
#define MSR_CSTAR 0xC0000083  /* Compatibility mode entry (unused) */
#define MSR_SFMASK 0xC0000084 /* RFLAGS mask */

/* System call handler type */
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5, uint64_t arg6);

/* Initialize syscall subsystem */
void syscall_init(void);

/* Register a syscall handler */
void syscall_register(int num, syscall_handler_t handler);

/* External assembly syscall entry point */
extern void syscall_entry(void);

/* Helper to write MSR */
static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

/* Helper to read MSR */
static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/* Userspace syscall wrapper (use in user programs) */
static inline int64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2,
                              uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    int64_t ret;
    register uint64_t r10 asm("r10") = arg4;
    register uint64_t r8 asm("r8") = arg5;

    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");

    return ret;
}

/* Userspace helper functions */
static inline void sys_exit(int status)
{
    syscall(SYS_EXIT, status, 0, 0, 0, 0);
    __builtin_unreachable();
}

static inline ssize_t sys_write(int fd, const void *buf, size_t count)
{
    return syscall(SYS_WRITE, fd, (uint64_t)buf, count, 0, 0);
}

static inline ssize_t sys_read(int fd, void *buf, size_t count)
{
    return syscall(SYS_READ, fd, (uint64_t)buf, count, 0, 0);
}

static inline int sys_getpid(void)
{
    return syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

static inline void sys_yield(void)
{
    syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

/* VFS syscall helpers */
static inline int sys_open(const char *path, int flags, int mode)
{
    return syscall(SYS_OPEN, (uint64_t)path, flags, mode, 0, 0);
}

static inline int sys_close(int fd)
{
    return syscall(SYS_CLOSE, fd, 0, 0, 0, 0);
}

static inline ssize_t sys_lseek(int fd, off_t offset, int whence)
{
    return syscall(SYS_LSEEK, fd, offset, whence, 0, 0);
}

static inline int sys_stat(const char *path, struct stat *statbuf)
{
    return syscall(SYS_STAT, (uint64_t)path, (uint64_t)statbuf, 0, 0, 0);
}

static inline int sys_fstat(int fd, struct stat *statbuf)
{
    return syscall(SYS_FSTAT, fd, (uint64_t)statbuf, 0, 0, 0);
}

static inline int sys_mkdir(const char *path, int mode)
{
    return syscall(SYS_MKDIR, (uint64_t)path, mode, 0, 0, 0);
}

static inline int sys_rmdir(const char *path)
{
    return syscall(SYS_RMDIR, (uint64_t)path, 0, 0, 0, 0);
}

static inline int sys_getdents(int fd, struct dirent *dirp, size_t count)
{
    return syscall(SYS_GETDENTS, fd, (uint64_t)dirp, count, 0, 0);
}

static inline int sys_unlink(const char *path)
{
    return syscall(SYS_UNLINK, (uint64_t)path, 0, 0, 0, 0);
}

#endif