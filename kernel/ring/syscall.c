/*
 * Copyright (c) 2026 Trollycat
 * System call implementation
 */

#include <thuban/syscall.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/gdt.h>

/* System call table */
static syscall_handler_t syscall_table[SYSCALL_MAX];

/* Forward declarations of syscall implementations */
static int64_t sys_exit_impl(uint64_t status, uint64_t arg2, uint64_t arg3,
                             uint64_t arg4, uint64_t arg5, uint64_t arg6);
static int64_t sys_write_impl(uint64_t fd, uint64_t buf, uint64_t count,
                              uint64_t arg4, uint64_t arg5, uint64_t arg6);
static int64_t sys_read_impl(uint64_t fd, uint64_t buf, uint64_t count,
                             uint64_t arg4, uint64_t arg5, uint64_t arg6);
static int64_t sys_getpid_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                               uint64_t arg4, uint64_t arg5, uint64_t arg6);
static int64_t sys_yield_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                              uint64_t arg4, uint64_t arg5, uint64_t arg6);

/*
 * Initialize syscall subsystem
 */
void syscall_init(void)
{
    /* Clear syscall table */
    memset(syscall_table, 0, sizeof(syscall_table));

    /* Register built-in syscalls */
    syscall_register(SYS_EXIT, sys_exit_impl);
    syscall_register(SYS_WRITE, sys_write_impl);
    syscall_register(SYS_READ, sys_read_impl);
    syscall_register(SYS_GETPID, sys_getpid_impl);
    syscall_register(SYS_YIELD, sys_yield_impl);

    /* Configure MSRs for SYSCALL/SYSRET */

    /* STAR: Set segment selectors
     * Bits 63-48: User code segment selector (0x18 | 3 = 0x1B)
     * Bits 47-32: Kernel code segment selector (0x08)
     */
    uint64_t star = ((uint64_t)(GDT_USER_CODE | 3) << 48) |
                    ((uint64_t)GDT_KERNEL_CODE << 32);
    wrmsr(MSR_STAR, star);

    /* LSTAR: Set syscall entry point */
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* SFMASK: Clear these RFLAGS bits on syscall
     * Clear IF (interrupts), DF (direction), TF (trap)
     */
    wrmsr(MSR_SFMASK, 0x200 | 0x400 | 0x100); /* IF | AC | TF */

    /* Enable SYSCALL/SYSRET in EFER (Extended Feature Enable Register) */
    uint64_t efer = rdmsr(0xC0000080);
    efer |= (1 << 0); /* SCE (System Call Extensions) bit */
    wrmsr(0xC0000080, efer);
}

/*
 * Register a syscall handler
 */
void syscall_register(int num, syscall_handler_t handler)
{
    if (num < 0 || num >= SYSCALL_MAX)
    {
        printf("[SYSCALL] Warning: Invalid syscall number %d\n", num);
        return;
    }

    syscall_table[num] = handler;
}

/*
 * Syscall dispatcher (called from assembly)
 * Arguments in: rdi, rsi, rdx, r10, r8, r9
 * Syscall number in: rax
 */
int64_t syscall_handler(uint64_t num, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /* Validate syscall number */
    if (num >= SYSCALL_MAX)
    {
        printf("[SYSCALL] Invalid syscall: %llu\n", num);
        return -1;
    }

    /* Check if handler is registered */
    if (syscall_table[num] == NULL)
    {
        printf("[SYSCALL] Unimplemented syscall: %llu\n", num);
        return -1;
    }

    /* Call the handler */
    return syscall_table[num](arg1, arg2, arg3, arg4, arg5, 0);
}

/* ============================================================================
 * Syscall Implementations
 * ============================================================================ */

/*
 * SYS_EXIT: Terminate current process
 */
static int64_t sys_exit_impl(uint64_t status, uint64_t arg2, uint64_t arg3,
                             uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    printf("\n[SYSCALL] Process exited with status %llu\n", status);

    /* TODO: When we have process management, actually terminate process */
    /* For now, just return to shell */

    return 0;
}

/*
 * SYS_WRITE: Write to file descriptor
 */
static int64_t sys_write_impl(uint64_t fd, uint64_t buf, uint64_t count,
                              uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg4;
    (void)arg5;
    (void)arg6;

    /* Basic validation */
    if (buf == 0)
    {
        return -1;
    }

    /* For now, only support stdout (fd 1) and stderr (fd 2) */
    if (fd != 1 && fd != 2)
    {
        printf("[SYSCALL] Write to unsupported fd: %llu\n", fd);
        return -1;
    }

    /* Write to console */
    const char *str = (const char *)buf;
    for (size_t i = 0; i < count; i++)
    {
        putchar(str[i]);
    }

    return (int64_t)count;
}

/*
 * SYS_READ: Read from file descriptor
 */
static int64_t sys_read_impl(uint64_t fd, uint64_t buf, uint64_t count,
                             uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg4;
    (void)arg5;
    (void)arg6;

    /* Basic validation */
    if (buf == 0 || count == 0)
    {
        return -1;
    }

    /* For now, only support stdin (fd 0) */
    if (fd != 0)
    {
        printf("[SYSCALL] Read from unsupported fd: %llu\n", fd);
        return -1;
    }

    /* Read from keyboard */
    char *buffer = (char *)buf;
    size_t bytes_read = 0;

    while (bytes_read < count)
    {
        int c = getchar();
        if (c == -1)
        {
            break;
        }

        buffer[bytes_read++] = (char)c;

        /* Stop on newline */
        if (c == '\n')
        {
            break;
        }
    }

    return (int64_t)bytes_read;
}

/*
 * SYS_GETPID: Get process ID
 */
static int64_t sys_getpid_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                               uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    /* TODO: Return actual PID when we have process management */
    return 1; /* For now, always return PID 1 */
}

/*
 * SYS_YIELD: Yield CPU to scheduler
 */
static int64_t sys_yield_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                              uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    /* TODO: Call scheduler when we have one */
    printf("[SYSCALL] Yield called\n");

    return 0;
}