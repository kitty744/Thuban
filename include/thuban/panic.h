/*
 * Copyright (c) 2026 Trollycat
 * Kernel panic and BSOD system
 */

#ifndef THUBAN_PANIC_H
#define THUBAN_PANIC_H

#include <thuban/interrupts.h>
#include <stdint.h>
#include <stddef.h>

/* Panic error codes */
#define PANIC_GENERAL_FAILURE 0x00000001
#define PANIC_PAGE_FAULT 0x00000050
#define PANIC_INVALID_OPCODE 0x0000006B
#define PANIC_DOUBLE_FAULT 0x0000007F
#define PANIC_STACK_OVERFLOW 0x00000077
#define PANIC_MEMORY_CORRUPTION 0x0000007A
#define PANIC_INACCESSIBLE_BOOT_DEVICE 0x0000007B
#define PANIC_KERNEL_MODE_EXCEPTION 0x0000001E
#define PANIC_IRQL_NOT_LESS_OR_EQUAL 0x0000000A
#define PANIC_DRIVER_IRQL_NOT_LESS 0x000000D1
#define PANIC_SYSTEM_SERVICE_EXCEPTION 0x0000003B
#define PANIC_MANUALLY_INITIATED_CRASH 0x000000E2

/* Panic with formatted message and error code */
void panic(uint32_t error_code, const char *fmt, ...) __attribute__((noreturn));

/* Panic from interrupt/exception context */
void panic_from_exception(struct registers *regs, uint32_t error_code, const char *message) __attribute__((noreturn));

/* BUG macros for kernel assertions */
#define BUG() panic(PANIC_MANUALLY_INITIATED_CRASH, "BUG at %s:%d in %s", __FILE__, __LINE__, __func__)

#define BUG_ON(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (condition)                                                         \
        {                                                                      \
            panic(PANIC_MANUALLY_INITIATED_CRASH, "BUG_ON(%s) at %s:%d in %s", \
                  #condition, __FILE__, __LINE__, __func__);                   \
        }                                                                      \
    } while (0)

/* WARN macros for non-fatal issues (prints but doesn't panic) */
#define WARN(fmt, ...)                                                                           \
    do                                                                                           \
    {                                                                                            \
        warn_print("WARNING at %s:%d in %s: " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

#define WARN_ON(condition)                                        \
    do                                                            \
    {                                                             \
        if (condition)                                            \
        {                                                         \
            warn_print("WARNING: condition '%s' at %s:%d in %s",  \
                       #condition, __FILE__, __LINE__, __func__); \
        }                                                         \
    } while (0)

/* Helper for WARN macros */
void warn_print(const char *fmt, ...);

/* Stack unwinding for panic display */
struct stack_frame
{
    struct stack_frame *rbp;
    uint64_t rip;
} __attribute__((packed));

#define MAX_STACK_FRAMES 10

#endif