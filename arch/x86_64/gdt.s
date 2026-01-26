[BITS 64]
section .text

global gdt_flush
global tss_flush

; flush GDT
; rdi = pointer to GDT pointer structure
gdt_flush:
    lgdt [rdi]
    
    ; reload data segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; reload code segment using far return
    pop rdi
    mov rax, 0x08
    push rax
    push rdi
    retfq

; flush TSS
tss_flush:
    mov ax, 0x28
    ltr ax
    ret