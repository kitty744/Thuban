/*
 * Copyright (c) 2026 Trollycat
 * Port I/O operations for Thuban
 */

#ifndef THUBAN_IO_H
#define THUBAN_IO_H

#include <stdint.h>

/*
 * Write's a byte to a port
 */
static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Read's a byte from a port
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write's a word to a port
 */
static inline void outw(uint16_t port, uint16_t value)
{
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Read's a word from a port
 */
static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Write's a dword to a port
 */
static inline void outl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Read's a dword from a port
 */
static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Wait's a tiny bit for I/O operation to complete
 */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif