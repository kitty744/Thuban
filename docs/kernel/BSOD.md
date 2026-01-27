# Kernel Panic Subsystem

## Overview

The Thuban kernel panic subsystem provides a robust mechanism for handling
fatal system errors. When an unrecoverable error occurs, the panic system
immediately halts normal operation, displays detailed diagnostic information
on a blue screen (BSOD), and allows the user to reboot the system.

## Table of Contents

1. [Architecture](#architecture)
2. [Error Codes](#error-codes)
3. [API Reference](#api-reference)
4. [Usage Examples](#usage-examples)
5. [Integration](#integration)
6. [Debugging](#debugging)
7. [Implementation Details](#implementation-details)

---

## Architecture

The panic subsystem consists of three main components:

### 1. Panic Handler (`panic.c`)

The core panic handling logic that:

- Disables interrupts to prevent reentrancy
- Switches to direct VGA buffer access
- Renders the blue screen interface
- Collects and displays system state
- Handles reboot sequencing

### 2. Exception Integration (`interrupts.c`)

CPU exception handlers that trigger panics:

- Maps hardware exceptions to panic codes
- Captures full register state
- Provides exception-specific context

### 3. Keyboard Integration (`ps2.c`)

Hardware polling for reboot trigger:

- Bypasses normal interrupt-driven input
- Flushes stale keyboard buffer
- Waits for deliberate user keypress

---

## Error Codes

Error codes follow Windows-style STOP codes for familiarity:

```c
/* General Errors */
PANIC_GENERAL_FAILURE          0x00000001  // Generic fatal error

/* Memory Errors */
PANIC_PAGE_FAULT               0x00000050  // Page fault exception
PANIC_STACK_OVERFLOW           0x00000077  // Stack segment fault
PANIC_MEMORY_CORRUPTION        0x0000007A  // Heap/memory corruption

/* CPU Exceptions */
PANIC_INVALID_OPCODE           0x0000006B  // Invalid instruction
PANIC_DOUBLE_FAULT             0x0000007F  // Double fault exception

/* Kernel Errors */
PANIC_KERNEL_MODE_EXCEPTION    0x0000001E  // General protection fault
PANIC_IRQL_NOT_LESS_OR_EQUAL   0x0000000A  // IRQL violation
PANIC_SYSTEM_SERVICE_EXCEPTION 0x0000003B  // System call error

/* Driver Errors */
PANIC_DRIVER_IRQL_NOT_LESS     0x000000D1  // Driver IRQL error
PANIC_INACCESSIBLE_BOOT_DEVICE 0x0000007B  // Critical device failure

/* Debug/Manual */
PANIC_MANUALLY_INITIATED_CRASH 0x000000E2  // BUG() macro triggered
```

### Error Code Guidelines

- **0x00000001-0x000000FF**: Kernel internal errors
- **0x00000100-0x000001FF**: Driver errors (reserved for future use)
- **0x00000200-0x000002FF**: Filesystem errors (reserved for future use)
- **0x00000300-0x000003FF**: Network errors (reserved for future use)

---

## API Reference

### Core Functions

#### panic()

```c
void panic(uint32_t error_code, const char *fmt, ...) __attribute__((noreturn));
```

**Description:**  
Triggers a kernel panic with a custom error code and formatted message.

**Parameters:**

- `error_code`: STOP code identifying the error type
- `fmt`: Printf-style format string
- `...`: Variable arguments for format string

**Example:**

```c
if (!critical_resource) {
    panic(PANIC_INACCESSIBLE_BOOT_DEVICE,
          "Failed to initialize %s after %d attempts",
          device_name, retry_count);
}
```

**Notes:**

- Never returns (marked `noreturn`)
- Disables interrupts immediately
- Safe to call from any context
- Uses stack-only allocation (no heap)

---

#### panic_from_exception()

```c
void panic_from_exception(struct registers *regs,
                         uint32_t error_code,
                         const char *message) __attribute__((noreturn));
```

**Description:**  
Triggers a panic from CPU exception context with full register dump.

**Parameters:**

- `regs`: Pointer to register state structure
- `error_code`: STOP code for the exception
- `message`: Human-readable exception name

**Example:**

```c
void isr_handler(struct registers *regs) {
    if (regs->int_no == 14) {  // Page fault
        panic_from_exception(regs, PANIC_PAGE_FAULT, "Page Fault");
    }
}
```

**Notes:**

- Automatically called by exception handlers
- Displays all CPU registers
- Includes stack trace
- Shows exception-specific information

---

### Assertion Macros

#### BUG()

```c
BUG()
```

**Description:**  
Unconditional kernel bug assertion. Triggers panic with file, line, and
function information.

**Example:**

```c
void impossible_path(void) {
    // This should never execute
    BUG();
}
```

**Equivalent to:**

```c
panic(PANIC_MANUALLY_INITIATED_CRASH,
      "BUG at %s:%d in %s", __FILE__, __LINE__, __func__);
```

---

#### BUG_ON()

```c
BUG_ON(condition)
```

**Description:**  
Conditional kernel bug assertion. Panics if condition evaluates to true.

**Parameters:**

- `condition`: Expression to evaluate

**Example:**

```c
void process_buffer(void *buf, size_t size) {
    BUG_ON(buf == NULL);
    BUG_ON(size == 0);
    BUG_ON(size > MAX_BUFFER_SIZE);

    // Safe to proceed
}
```

**When to use:**

- Null pointer checks in critical paths
- Range validation
- State invariant checks
- Impossible condition detection

---

### Warning Macros

#### WARN()

```c
WARN(fmt, ...)
```

**Description:**  
Prints a warning message in yellow but continues execution.

**Parameters:**

- `fmt`: Printf-style format string
- `...`: Variable arguments

**Example:**

```c
if (suspicious_value > threshold) {
    WARN("Suspicious value detected: %d (threshold: %d)",
         suspicious_value, threshold);
}
// Execution continues here
```

---

#### WARN_ON()

```c
WARN_ON(condition)
```

**Description:**  
Prints a warning if condition is true but continues execution.

**Parameters:**

- `condition`: Expression to evaluate

**Example:**

```c
WARN_ON(allocation_size > 1024 * 1024);  // Warn on >1MB alloc
WARN_ON(reference_count < 0);            // Warn on negative refcount
```

**When to use:**

- Deprecated API usage
- Potential resource leaks
- Performance warnings
- Non-critical invariant violations

---

## Usage Examples

### Example 1: Driver Initialization Failure

```c
int init_critical_driver(void) {
    if (!hardware_present()) {
        panic(PANIC_INACCESSIBLE_BOOT_DEVICE,
              "Critical hardware not detected");
    }

    if (!hardware_initialize()) {
        panic(PANIC_INACCESSIBLE_BOOT_DEVICE,
              "Hardware initialization failed: timeout");
    }

    return 0;
}
```

### Example 2: Memory Corruption Detection

```c
void validate_heap_integrity(void) {
    struct heap_block *block = heap_first_block();

    while (block) {
        if (block->magic != HEAP_MAGIC) {
            panic(PANIC_MEMORY_CORRUPTION,
                  "Heap corruption detected at 0x%p (magic: 0x%x)",
                  block, block->magic);
        }
        block = block->next;
    }
}
```

### Example 3: Kernel Invariant Checks

```c
void scheduler_switch_context(struct task *next) {
    BUG_ON(next == NULL);
    BUG_ON(next->state != TASK_READY);
    BUG_ON(!interrupts_disabled());

    // Safe to switch context
    switch_to(next);
}
```

### Example 4: Non-Fatal Warnings

```c
void *kmalloc(size_t size, int flags) {
    WARN_ON(size == 0);
    WARN_ON(size > 1024 * 1024 * 10);  // >10MB is suspicious

    if (size == 0) {
        return NULL;
    }

    return allocate_memory(size, flags);
}
```

### Example 5: Resource Leak Detection

```c
void free_resource(struct resource *res) {
    if (res->refcount != 0) {
        WARN("Freeing resource with non-zero refcount: %d",
             res->refcount);
    }

    kfree(res);
}
```

---

## Integration

### Using in Drivers

**drivers/example/driver.c:**

```c
#include <thuban/panic.h>

int driver_init(void) {
    void *hw_base = map_hardware(DEVICE_BASE);
    BUG_ON(hw_base == NULL);

    if (!device_ready(hw_base)) {
        panic(PANIC_INACCESSIBLE_BOOT_DEVICE,
              "Device %s failed to initialize", DEVICE_NAME);
    }

    return 0;
}
```

---

## Debugging

### Analyzing a Panic Screen

When a panic occurs, the BSOD displays:

```
*** STOP: A fatal system error has occurred ***
----------------------------------------------------------------
Error Code: 0x0000007B

Failed to initialize critical device: timeout

----------------------------------------------------------------

                      REGISTER DUMP
----------------------------------------------------------------
RAX: 0x0000000000000001  RBX: 0x0000000000000000
RCX: 0x00000000DEADBEEF  RDX: 0x0000000000000000
...
RIP: 0xFFFFFFFF80012345  CS:  0x0008
----------------------------------------------------------------

                       STACK TRACE
----------------------------------------------------------------
  [0] RIP: 0xFFFFFFFF80012345  RBP: 0xFFFFFFFF80100F20
  [1] RIP: 0xFFFFFFFF80005678  RBP: 0xFFFFFFFF80100F40
...
```

### What to Check

1. **Error Code**: Identifies the category of error
2. **RIP (Instruction Pointer)**: Where the panic occurred
3. **Stack Trace**: Call chain leading to panic
4. **Registers**: CPU state at panic time

### Using with GDB

To match RIP addresses to source code:

```bash
# Get symbol at address
(gdb) info symbol 0xFFFFFFFF80012345

# Get line info
(gdb) list *0xFFFFFFFF80012345

# Disassemble around panic
(gdb) disassemble 0xFFFFFFFF80012345
```

### Common Panic Scenarios

**Page Fault (0x00000050):**

- Check RIP for faulting instruction
- Check RSI/RDI for bad pointers
- Verify page tables are set up correctly

**General Protection Fault (0x0000001E):**

- Invalid memory access
- Segment violation
- Null pointer dereference

**Double Fault (0x0000007F):**

- Stack overflow
- Invalid exception handler
- Recursive exceptions

---

## Implementation Details

### BSOD Rendering

The panic system uses **direct VGA buffer access** at `0xB8000`:

```c
// Blue background (VGA color 1), white text (VGA color 15)
vga_set_color(COLOR_WHITE, COLOR_BLUE);
vga_clear_screen();
```

**Why direct access?**

- Bypasses potentially corrupted kernel state
- Works even if interrupts are broken
- No dependency on working stdio
- Guaranteed to display something

### Stack Unwinding

Stack traces are generated by walking the RBP chain:

```c
struct stack_frame {
    struct stack_frame *rbp;  // Previous frame pointer
    uint64_t rip;             // Return address
};
```

**Limitations:**

- Requires frame pointers (compile with `-fno-omit-frame-pointer`)
- Limited to 10 frames to prevent infinite loops
- May fail if stack is corrupted
- Validates pointers to prevent additional faults

### Reboot Mechanism

The panic system reboots in three stages:

1. **Keyboard Controller Reset** (preferred):

   ```c
   outb(0x64, 0xFE);  // Send reset command to keyboard controller
   ```

2. **Triple Fault** (fallback):

   ```c
   asm volatile("int $0xFF");  // Trigger triple fault
   ```

3. **Halt** (last resort):
   ```c
   while (1) { asm volatile("hlt"); }
   ```

### Interrupt Safety

All panic paths disable interrupts:

```c
asm volatile("cli");  // Clear interrupt flag
```

This prevents:

- Reentrancy during panic
- Timer interrupts during BSOD display
- Keyboard interrupts corrupting state

### Memory Usage

The panic system uses **zero heap allocations**:

- All buffers are stack-allocated
- Direct VGA buffer writes
- No kmalloc/kfree calls
- Safe to use in any context

---

## Best Practices

### When to Use panic()

✅ **DO use panic() for:**

- Unrecoverable hardware failures
- Critical resource unavailable
- Kernel data structure corruption
- Security violations
- Hardware integrity failures

❌ **DON'T use panic() for:**

- User input errors
- Recoverable driver errors
- Network failures
- Disk errors
- Non-critical resource exhaustion

### When to Use BUG_ON()

✅ **DO use BUG_ON() for:**

- Null pointer checks in critical code
- Kernel invariant violations
- Impossible conditions
- Internal logic errors

❌ **DON'T use BUG_ON() for:**

- Input validation
- Hardware status checks
- Resource availability
- User-provided data

### When to Use WARN()

✅ **DO use WARN() for:**

- Deprecated API usage
- Resource leaks detected
- Performance issues
- Suspicious but recoverable states

❌ **DON'T use WARN() for:**

- Expected error conditions
- Debug messages
- Informational logging
- Frequent events

---

## Testing

### Manual Test Cases

**Test 1: Division by Zero**

```c
volatile int x = 1;
volatile int y = 0;
volatile int z = x / y;
```

**Test 2: Invalid Opcode**

```c
asm volatile("ud2");
```

**Test 3: Page Fault**

```c
*(volatile int*)0xDEADBEEF = 42;
```

**Test 4: Manual Panic**

```c
panic(PANIC_MANUALLY_INITIATED_CRASH, "Test panic");
```

**Test 5: BUG Macro**

```c
BUG();
```

---

## References

### Related Subsystems

- **Interrupts**: `kernel/interrupts.c`, `include/thuban/interrupts.h`
- **VGA Driver**: `drivers/video/vga.c`, `include/thuban/vga.h`
- **Keyboard**: `drivers/keyboard/ps2.c`, `include/thuban/keyboard.h`
- **Memory**: `mm/pmm.c`, `mm/vmm.c`, `mm/heap.c`

### External Documentation

- Intel 64 and IA-32 Architectures Software Developer's Manual (Exception Handling)
- VGA Hardware Programming (INT 10h, Memory-Mapped I/O)
- PS/2 Keyboard Protocol (Scancodes, Port 0x60/0x64)

---

## See Also

- `BUG(9)` - Kernel bug macro documentation
- `printk(9)` - Kernel logging (future)
- `oops(9)` - Non-fatal kernel errors (future)
- `dump_stack(9)` - Stack trace utility (future)

---

**Maintainer:** Trollycat  
**License:** MIT  
**Last Updated:** January 2026
