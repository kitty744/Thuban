/*
 * Copyright (c) 2026 Trollycat
 * Interrupt handling implementation
 */

#include <thuban/interrupts.h>
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
        printf("\n[EXCEPTION] %s (interrupt %llu)\n",
               exception_messages[regs->int_no], regs->int_no);
        printf("Error code: 0x%llx\n", regs->err_code);
        printf("RIP: 0x%llx\n", regs->rip);
        printf("CS: 0x%llx  RFLAGS: 0x%llx\n", regs->cs, regs->rflags);
        printf("RSP: 0x%llx  SS: 0x%llx\n", regs->rsp, regs->ss);

        // halt system on exception
        printf("\nSystem halted\n");
        while (1)
        {
            asm volatile("cli; hlt");
        }
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