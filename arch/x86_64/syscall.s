[BITS 64]
section .text

global syscall_entry
extern syscall_handler
extern gdt_set_kernel_stack

; Syscall entry point
; When userspace executes SYSCALL instruction:
;   - RCX = return RIP (saved by CPU)
;   - R11 = RFLAGS (saved by CPU)
;   - RAX = syscall number
;   - RDI, RSI, RDX, R10, R8, R9 = syscall arguments
;
; We need to:
;   - Switch to kernel stack
;   - Save user context
;   - Call C handler
;   - Restore user context
;   - Return via SYSRET

syscall_entry:
    ; Save user stack pointer
    mov [rel user_rsp], rsp
    
    ; Switch to kernel stack
    ; TODO: When we have per-process kernel stacks, use TSS.rsp0
    ; For now, use a static kernel stack
    mov rsp, kernel_syscall_stack_top
    
    ; Build a fake interrupt frame on kernel stack
    ; This makes it easier to debug and matches exception handling
    push qword 0x20 | 3         ; User data segment (SS)
    push qword [rel user_rsp]   ; User RSP
    push r11                     ; RFLAGS (saved by SYSCALL)
    push qword 0x18 | 3         ; User code segment (CS)
    push rcx                     ; Return RIP (saved by SYSCALL)
    
    ; Save all registers (matching struct registers layout)
    push rax  ; Will be return value
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
    
    ; Arguments to syscall_handler:
    ; rdi = syscall number (rax)
    ; rsi = arg1 (rdi)
    ; rdx = arg2 (rsi)
    ; rcx = arg3 (rdx)
    ; r8  = arg4 (r10)
    ; r9  = arg5 (r8)
    
    mov rdi, rax    ; syscall number
    mov rsi, rdi    ; arg1 (original rdi)
    mov rdx, rsi    ; arg2 (original rsi)
    mov rcx, rdx    ; arg3 (original rdx)
    mov r8, r10     ; arg4 (original r10)
    mov r9, r8      ; arg5 (original r8)
    
    ; Align stack to 16 bytes (required by System V ABI)
    mov rbp, rsp
    and rsp, ~0xF
    
    ; Call C handler
    call syscall_handler
    
    ; Restore stack
    mov rsp, rbp
    
    ; Return value in rax
    mov [rsp], rax  ; Overwrite saved rax with return value
    
    ; Restore all registers
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
    pop rax  ; Return value
    
    ; Pop interrupt frame
    add rsp, 8      ; Skip RIP (rcx already has it)
    add rsp, 8      ; Skip CS
    pop r11         ; Restore RFLAGS
    pop rsp         ; Restore user stack
    add rsp, 8      ; Skip SS
    
    ; Return to userspace
    ; RCX = return RIP (from SYSCALL)
    ; R11 = RFLAGS (from SYSCALL)
    o64 sysret

section .bss
align 16
    resb 8192  ; 8KB kernel stack for syscalls
kernel_syscall_stack_top:

align 8
user_rsp: resq 1