/*
 * Copyright (c) 2026 Trollycat
 * User mode transition implementation
 */

#include <thuban/usermode.h>
#include <thuban/gdt.h>
#include <thuban/stdio.h>
#include <thuban/string.h>
#include <thuban/heap.h>

/*
 * Allocate and initialize a user mode stack
 */
void *create_user_stack(size_t size)
{
    /* Allocate memory for stack */
    void *stack = malloc(size);
    if (!stack)
    {
        printf("[USERMODE] Failed to allocate user stack (%zu bytes)\n", size);
        return NULL;
    }

    /* Zero out the stack for safety */
    memset(stack, 0, size);

    /* Return pointer to TOP of stack (x86_64 stacks grow downward) */
    return (void *)((uint64_t)stack + size);
}

/*
 * Transition from ring 0 (kernel) to ring 3 (user mode)
 */
void jump_to_usermode(void (*entry_point)(void), void *user_stack)
{
    printf("[USERMODE] Transitioning to ring 3\n");
    printf("[USERMODE]   Entry point: 0x%p\n", entry_point);
    printf("[USERMODE]   User stack:  0x%p\n", user_stack);

    /*
     * Set kernel stack in TSS (Task State Segment)
     * When syscalls or interrupts occur in user mode, CPU will load this stack
     *
     * This uses the kernel stack from boot.s
     */
    extern uint8_t stack_top[]; /* Defined in boot.s */
    gdt_set_kernel_stack((uint64_t)stack_top);

    printf("[USERMODE] Kernel stack set to 0x%p\n", stack_top);
    printf("[USERMODE] Jumping to user mode...\n\n");

    /*
     * Call assembly routine to perform ring transition
     * Arguments:
     *   - Entry point (user function to execute)
     *   - User stack pointer
     *   - User code segment (GDT_USER_CODE | DPL=3)
     *   - User data segment (GDT_USER_DATA | DPL=3)
     */
    enter_usermode((uint64_t)entry_point,
                   (uint64_t)user_stack,
                   GDT_USER_CODE | 3,  /* 0x18 | 3 = 0x1B */
                   GDT_USER_DATA | 3); /* 0x20 | 3 = 0x23 */

    /*
     * Should NEVER reach here!
     * If we do, something went very wrong with the ring transition
     */
    printf("[USERMODE] FATAL: Returned from user mode!\n");
    while (1)
    {
        asm volatile("hlt");
    }
}