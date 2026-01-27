/*
 * Copyright (c) 2026 Trollycat
 * Spinlock implementation
 */

#include <thuban/spinlock.h>
#include <thuban/stdio.h>

/*
 * Save RFLAGS and disable interrupts
 */
static inline uint64_t save_flags_and_cli(void)
{
    uint64_t flags;
    asm volatile(
        "pushfq\n" /* Push RFLAGS onto stack */
        "pop %0\n" /* Pop into flags variable */
        "cli\n"    /* Disable interrupts */
        : "=r"(flags)
        :
        : "memory");
    return flags;
}

/*
 * Restore RFLAGS (and interrupt state)
 */
static inline void restore_flags(uint64_t flags)
{
    asm volatile(
        "push %0\n" /* Push flags onto stack */
        "popfq\n"   /* Pop into RFLAGS */
        :
        : "r"(flags)
        : "memory", "cc");
}

/*
 * Initialize a spinlock
 */
void spin_lock_init(spinlock_t *lock, const char *name)
{
    lock->flags = 0;
    lock->locked = 0;
    lock->name = name;
}

/*
 * Acquire a spinlock
 */
void spin_lock(spinlock_t *lock)
{
    /* Save interrupt state and disable interrupts */
    lock->flags = save_flags_and_cli();

    /*
     * On single-core systems, disabling interrupts is sufficient
     * On SMP systems, we would spin here using atomic operations:
     *
     * while (__sync_lock_test_and_set(&lock->locked, 1)) {
     *     while (lock->locked) {
     *         asm volatile("pause");
     *     }
     * }
     */

    /* For now, just mark as locked */
    lock->locked = 1;
}

/*
 * Release a spinlock
 */
void spin_unlock(spinlock_t *lock)
{
    /* Mark as unlocked */
    lock->locked = 0;

    /*
     * On SMP systems, we would use atomic release:
     * __sync_lock_release(&lock->locked);
     */

    /* Restore interrupt state */
    restore_flags(lock->flags);
}

/*
 * Try to acquire a spinlock without blocking
 */
int spin_trylock(spinlock_t *lock)
{
    /* Save interrupt state and disable interrupts */
    uint64_t flags = save_flags_and_cli();

    /* Check if already locked */
    if (lock->locked)
    {
        /* Failed to acquire - restore interrupts */
        restore_flags(flags);
        return 0;
    }

    /* Acquired successfully */
    lock->flags = flags;
    lock->locked = 1;
    return 1;
}

/*
 * Check if spinlock is locked
 */
int spin_is_locked(spinlock_t *lock)
{
    return lock->locked;
}