/*
 * Copyright (c) 2026 Trollycat
 * Spinlock implementation for Thuban
 *
 * Single-core implementation:
 * Spinlocks protect critical sections by disabling interrupts.
 * In future SMP systems, this will use atomic operations.
 */

#ifndef THUBAN_SPINLOCK_H
#define THUBAN_SPINLOCK_H

#include <stdint.h>

/*
 * Spinlock structure
 * For single-core: tracks interrupt state
 * For SMP: would contain atomic lock variable
 */
typedef struct spinlock
{
    uint64_t flags;   /* Saved interrupt flags */
    int locked;       /* Lock state (0 = unlocked, 1 = locked) */
    const char *name; /* Lock name for debugging */
} spinlock_t;

/*
 * Static initializer for spinlocks
 * Usage: spinlock_t my_lock = SPINLOCK_INIT;
 */
#define SPINLOCK_INIT {.flags = 0, .locked = 0, .name = NULL}

/*
 * Named static initializer for spinlocks
 * Usage: spinlock_t my_lock = SPINLOCK_INIT_NAMED("my_lock");
 */
#define SPINLOCK_INIT_NAMED(lock_name) \
    {.flags = 0, .locked = 0, .name = lock_name}

/*
 * Initialize a spinlock at runtime
 *
 * Parameters:
 *   lock - Pointer to spinlock structure
 *   name - Name for debugging (can be NULL)
 */
void spin_lock_init(spinlock_t *lock, const char *name);

/*
 * Acquire a spinlock
 * Disables interrupts and saves flags
 * Blocks until lock is available (spin-waits in SMP)
 *
 * Parameters:
 *   lock - Pointer to spinlock structure
 */
void spin_lock(spinlock_t *lock);

/*
 * Release a spinlock
 * Restores interrupt state from when lock was acquired
 *
 * Parameters:
 *   lock - Pointer to spinlock structure
 */
void spin_unlock(spinlock_t *lock);

/*
 * Try to acquire a spinlock without blocking
 *
 * Parameters:
 *   lock - Pointer to spinlock structure
 *
 * Returns:
 *   1 if lock was acquired
 *   0 if lock was already held
 */
int spin_trylock(spinlock_t *lock);

/*
 * Check if spinlock is currently held
 *
 * Parameters:
 *   lock - Pointer to spinlock structure
 *
 * Returns:
 *   1 if locked
 *   0 if unlocked
 */
int spin_is_locked(spinlock_t *lock);

#endif