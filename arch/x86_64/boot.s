[BITS 32]
section .boot

KERNEL_VIRT_OFFSET equ 0xFFFFFFFF80000000

align 8
multiboot_header_start:
    dd 0xe85250d6
    dd 0
    dd multiboot_header_end - multiboot_header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start))
    
    dw 0
    dw 0
    dd 8
multiboot_header_end:

global start
global p4_table
global p3_table
global p2_table
extern kmain

start:
    mov esp, stack_top - KERNEL_VIRT_OFFSET
    mov ebp, eax
    mov esi, ebx
    
    mov edi, p4_table - KERNEL_VIRT_OFFSET
    mov ecx, 3072
    xor eax, eax
    cld
    rep stosd
    
    mov eax, p3_table - KERNEL_VIRT_OFFSET
    or eax, 0b11
    mov edi, p4_table - KERNEL_VIRT_OFFSET
    mov [edi], eax
    mov [edi + 511 * 8], eax
    
    mov eax, p2_table - KERNEL_VIRT_OFFSET
    or eax, 0b11
    mov edi, p3_table - KERNEL_VIRT_OFFSET
    mov [edi], eax
    mov [edi + 510 * 8], eax
    
    mov edi, p2_table - KERNEL_VIRT_OFFSET
    xor ecx, ecx
.loop_p2:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [edi + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .loop_p2
    
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    mov eax, p4_table - KERNEL_VIRT_OFFSET
    mov cr3, eax
    
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    lgdt [gdt64.pointer_phys - KERNEL_VIRT_OFFSET]
    jmp 0x08:(long_mode_start - KERNEL_VIRT_OFFSET)

[BITS 64]
section .text

long_mode_start:
    mov rax, .higher_half
    jmp rax

.higher_half:
    mov rax, gdt64.pointer_virt
    lgdt [rax]
    
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov rsp, stack_top
    
    mov edi, ebp
    mov esi, esi
    
    call kmain
    
    cli
.hang:
    hlt
    jmp .hang

section .rodata
align 8
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.pointer_phys:
    dw $ - gdt64 - 1
    dq gdt64 - KERNEL_VIRT_OFFSET
.pointer_virt:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096
stack_bottom:
    resb 4096 * 16
stack_top: