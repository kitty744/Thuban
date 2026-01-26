/*
 * Copyright (c) 2026 Trollycat
 * Interrupt handling for Thuban
 */

#ifndef THUBAN_INTERRUPTS_H
#define THUBAN_INTERRUPTS_H

#include <stdint.h>

// PIC ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

// PIC commands
#define PIC_EOI 0x20

// register state pushed by ISR stub
struct registers
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

// exception ISRs (declared in idt.s)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// IRQ handlers (declared in idt.s)
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

// interrupt handler type
typedef void (*irq_handler_t)(struct registers *regs);

// initialize interrupt system
void interrupts_init(void);

// install custom IRQ handler
void irq_install_handler(int irq, irq_handler_t handler);

// uninstall IRQ handler
void irq_uninstall_handler(int irq);

// enable interrupts
static inline void interrupts_enable(void)
{
    asm volatile("sti");
}

// disable interrupts
static inline void interrupts_disable(void)
{
    asm volatile("cli");
}

#endif