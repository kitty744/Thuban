;
; Copyright (c) 2026 Trollycat
; User mode transition assembly
;
; This performs the actual CPU privilege level change from ring 0 to ring 3
;

[BITS 64]
section .text

global enter_usermode

;
; Enter user mode (ring 3) from kernel mode (ring 0)
;
; Arguments (System V ABI):
;   rdi = entry point (user function address)
;   rsi = user stack pointer (top of stack)
;   rdx = user code segment selector (0x1B = 0x18 | 3)
;   rcx = user data segment selector (0x23 = 0x20 | 3)
;
; This function uses IRETQ to perform the ring transition:
;   - IRETQ expects a specific stack frame
;   - Changes CPL (Current Privilege Level) from 0 to 3
;   - Loads new segments and stack
;
enter_usermode:
    ; Disable interrupts during setup
    cli
    
    ; Switch data segments to user mode FIRST
    ; This is safe because we're still in ring 0
    mov ax, cx              ; cx = user data segment (0x23)
    mov ds, ax              ; Set DS (data segment)
    mov es, ax              ; Set ES (extra segment)
    mov fs, ax              ; Set FS
    mov gs, ax              ; Set GS
    
    ;
    ; Build IRETQ stack frame on current kernel stack
    ;
    ; IRETQ expects (bottom to top):
    ;   [rsp+32] SS      - Stack segment selector
    ;   [rsp+24] RSP     - Stack pointer
    ;   [rsp+16] RFLAGS  - CPU flags
    ;   [rsp+8]  CS      - Code segment selector
    ;   [rsp+0]  RIP     - Instruction pointer
    ;
    ; After IRETQ executes, CPU will:
    ;   - Load SS:RSP (switch to user stack)
    ;   - Load CS:RIP (jump to user code)
    ;   - Restore RFLAGS
    ;   - Change CPL from 0 to 3
    ;
    
    push rcx                ; SS (user data segment = 0x23)
    push rsi                ; RSP (user stack pointer)
    
    ; Prepare RFLAGS
    pushfq                  ; Push current RFLAGS
    pop rax                 ; Pop into RAX to modify
    or rax, 0x200           ; Set IF (Interrupt Flag, bit 9) - enable interrupts
    and rax, ~0x3000        ; Clear IOPL (I/O Privilege Level, bits 12-13)
    push rax                ; Push modified RFLAGS
    
    push rdx                ; CS (user code segment = 0x1B)
    push rdi                ; RIP (entry point)
    
    ; Perform the ring transition!
    ; This atomically:
    ;   1. Pops RIP, CS, RFLAGS, RSP, SS from stack
    ;   2. Switches CPL from 0 to 3 (based on CS selector's RPL)
    ;   3. Jumps to user code
    iretq
    
    ; Should NEVER reach here - iretq doesn't return
    ; If we somehow get here, trigger invalid opcode
    ud2