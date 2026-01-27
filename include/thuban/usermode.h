/*
 * Copyright (c) 2026 Trollycat
 * User mode transition and management
 */

#ifndef THUBAN_USERMODE_H
#define THUBAN_USERMODE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Transition from kernel mode (ring 0) to user mode (ring 3)
 * and execute a function
 *
 * Parameters:
 *   entry_point - Function to execute in user mode
 *   user_stack  - Stack pointer for user mode (top of stack)
 *
 * This function does NOT return - it jumps to user mode
 */
void jump_to_usermode(void (*entry_point)(void), void *user_stack) __attribute__((noreturn));

/*
 * Allocate and initialize a user mode stack
 *
 * Parameters:
 *   size - Size of stack in bytes (typically 4KB-8KB)
 *
 * Returns:
 *   Pointer to TOP of stack (stacks grow downward)
 *   NULL on allocation failure
 */
void *create_user_stack(size_t size);

/*
 * External assembly function - performs the actual ring transition
 * DO NOT CALL DIRECTLY - use jump_to_usermode() instead
 */
extern void enter_usermode(uint64_t entry, uint64_t stack,
                           uint64_t code_seg, uint64_t data_seg);

#endif