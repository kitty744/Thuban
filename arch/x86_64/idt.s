[BITS 64]
section .text

global idt_flush
extern isr_handler
extern irq_handler

; flush IDT
idt_flush:
    lidt [rdi]
    ret

; macro for ISRs without error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0
    push %1
    jmp isr_common_stub
%endmacro

; macro for ISRs with error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1
    jmp isr_common_stub
%endmacro

; macro for IRQs
%macro IRQ 2
global irq%1
irq%1:
    push 0
    push %2
    jmp irq_common_stub
%endmacro

; CPU exception ISRs (0-31)
ISR_NOERRCODE 0   ; divide by zero
ISR_NOERRCODE 1   ; debug
ISR_NOERRCODE 2   ; non-maskable interrupt
ISR_NOERRCODE 3   ; breakpoint
ISR_NOERRCODE 4   ; overflow
ISR_NOERRCODE 5   ; bound range exceeded
ISR_NOERRCODE 6   ; invalid opcode
ISR_NOERRCODE 7   ; device not available
ISR_ERRCODE   8   ; double fault
ISR_NOERRCODE 9   ; coprocessor segment overrun
ISR_ERRCODE   10  ; invalid TSS
ISR_ERRCODE   11  ; segment not present
ISR_ERRCODE   12  ; stack-segment fault
ISR_ERRCODE   13  ; general protection fault
ISR_ERRCODE   14  ; page fault
ISR_NOERRCODE 15  ; reserved
ISR_NOERRCODE 16  ; x87 floating-point exception
ISR_ERRCODE   17  ; alignment check
ISR_NOERRCODE 18  ; machine check
ISR_NOERRCODE 19  ; SIMD floating-point exception
ISR_NOERRCODE 20  ; virtualization exception
ISR_NOERRCODE 21  ; reserved
ISR_NOERRCODE 22  ; reserved
ISR_NOERRCODE 23  ; reserved
ISR_NOERRCODE 24  ; reserved
ISR_NOERRCODE 25  ; reserved
ISR_NOERRCODE 26  ; reserved
ISR_NOERRCODE 27  ; reserved
ISR_NOERRCODE 28  ; reserved
ISR_NOERRCODE 29  ; reserved
ISR_ERRCODE   30  ; security exception
ISR_NOERRCODE 31  ; reserved

; IRQ handlers (32-47)
IRQ 0, 32   ; PIT timer
IRQ 1, 33   ; keyboard
IRQ 2, 34   ; cascade
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; floppy
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOS RTC
IRQ 9, 41   ; free
IRQ 10, 42  ; free
IRQ 11, 43  ; free
IRQ 12, 44  ; PS/2 mouse
IRQ 13, 45  ; FPU
IRQ 14, 46  ; primary ATA
IRQ 15, 47  ; secondary ATA

; common ISR stub
isr_common_stub:
    ; save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; call C handler
    mov rdi, rsp
    call isr_handler
    
    ; restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; remove error code and interrupt number
    add rsp, 16
    
    iretq

; common IRQ stub
irq_common_stub:
    ; save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; call C handler
    mov rdi, rsp
    call irq_handler
    
    ; restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; remove error code and interrupt number
    add rsp, 16
    
    iretq