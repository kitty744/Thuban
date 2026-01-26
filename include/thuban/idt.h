/*
 * Copyright (c) 2026 Trollycat
 * Interrupt Descriptor Table for Thuban
 */

#ifndef THUBAN_IDT_H
#define THUBAN_IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

// IDT gate types
#define IDT_GATE_INTERRUPT 0x8E
#define IDT_GATE_TRAP 0x8F
#define IDT_GATE_TASK 0x85

struct idt_entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct interrupt_frame
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

// initialize IDT
void idt_init(void);

// set IDT gate
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr);

// external assembly function
extern void idt_flush(uint64_t idt_ptr);

#endif