/*
 * Copyright (c) 2026 Trollycat
 * Interrupt handling implementation
 */

#include <thuban/interrupts.h>
#include <thuban/panic.h>
#include <thuban/stdio.h>
#include <thuban/io.h>

static irq_handler_t irq_handlers[16] = {0};

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"};

/* Map exception numbers to panic error codes */
static const uint32_t exception_error_codes[] = {
    PANIC_GENERAL_FAILURE,       // 0: Division By Zero
    PANIC_GENERAL_FAILURE,       // 1: Debug
    PANIC_GENERAL_FAILURE,       // 2: Non Maskable Interrupt
    PANIC_GENERAL_FAILURE,       // 3: Breakpoint
    PANIC_GENERAL_FAILURE,       // 4: Overflow
    PANIC_GENERAL_FAILURE,       // 5: Bound Range Exceeded
    PANIC_INVALID_OPCODE,        // 6: Invalid Opcode
    PANIC_GENERAL_FAILURE,       // 7: Device Not Available
    PANIC_DOUBLE_FAULT,          // 8: Double Fault
    PANIC_GENERAL_FAILURE,       // 9: Coprocessor Segment Overrun
    PANIC_GENERAL_FAILURE,       // 10: Invalid TSS
    PANIC_GENERAL_FAILURE,       // 11: Segment Not Present
    PANIC_STACK_OVERFLOW,        // 12: Stack-Segment Fault
    PANIC_KERNEL_MODE_EXCEPTION, // 13: General Protection Fault
    PANIC_PAGE_FAULT,            // 14: Page Fault
    PANIC_GENERAL_FAILURE,       // 15: Reserved
    PANIC_GENERAL_FAILURE,       // 16: x87 Floating-Point Exception
    PANIC_GENERAL_FAILURE,       // 17: Alignment Check
    PANIC_GENERAL_FAILURE,       // 18: Machine Check
    PANIC_GENERAL_FAILURE,       // 19: SIMD Floating-Point Exception
    PANIC_GENERAL_FAILURE,       // 20: Virtualization Exception
    PANIC_GENERAL_FAILURE,       // 21-31: Reserved
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE,
    PANIC_GENERAL_FAILURE};

/*
 * Remap's the PIC to avoid conflicts with CPU exceptions
 */
static void pic_remap(void)
{
    // save masks
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // start initialization sequence
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // set vector offsets
    outb(PIC1_DATA, 0x20); // master PIC offset to 32
    outb(PIC2_DATA, 0x28); // slave PIC offset to 40

    // tell master PIC there's a slave at IRQ2
    outb(PIC1_DATA, 0x04);

    // tell slave PIC its cascade identity
    outb(PIC2_DATA, 0x02);

    // set 8086 mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // restore masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

/*
 * Send's End Of Interrupt signal to PIC
 */
static void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/*
 * ISR handler called from assembly
 */
void isr_handler(struct registers *regs)
{
    if (regs->int_no < 32)
    {
        /* CPU exception - trigger panic with BSOD */
        uint32_t error_code = exception_error_codes[regs->int_no];
        const char *message = exception_messages[regs->int_no];

        panic_from_exception(regs, error_code, message);
    }
}

/*
 * IRQ handler called from assembly
 */
void irq_handler(struct registers *regs)
{
    int irq = regs->int_no - 32;

    if (irq >= 0 && irq < 16)
    {
        if (irq_handlers[irq])
        {
            irq_handlers[irq](regs);
        }
    }

    pic_send_eoi(irq);
}

/*
 * Initialize's interrupt system
 */
void interrupts_init(void)
{
    pic_remap();
}

/*
 * Install's custom IRQ handler
 */
void irq_install_handler(int irq, irq_handler_t handler)
{
    if (irq >= 0 && irq < 16)
    {
        irq_handlers[irq] = handler;
    }
}

/*
 * Uninstall's IRQ handler
 */
void irq_uninstall_handler(int irq)
{
    if (irq >= 0 && irq < 16)
    {
        irq_handlers[irq] = NULL;
    }
}