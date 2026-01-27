# System Call Subsystem

## Overview

The Thuban system call subsystem provides a mechanism for user-space programs
(Ring 3) to request services from the kernel (Ring 0). System calls are the
primary interface between user applications and kernel functionality.

## Table of Contents

1. [Architecture](#architecture)
2. [System Call Numbers](#system-call-numbers)
3. [API Reference](#api-reference)
4. [Usage Examples](#usage-examples)
5. [Implementation Details](#implementation-details)
6. [Integration](#integration)

---

## Architecture

The syscall subsystem consists of three main components:

### 1. Hardware Layer (MSRs)

Model Specific Registers configure the SYSCALL/SYSRET instructions:

- **MSR_STAR (0xC0000081)**: Segment selectors for kernel/user transitions
- **MSR_LSTAR (0xC0000082)**: Entry point address (syscall_entry)
- **MSR_SFMASK (0xC0000084)**: RFLAGS bits to clear on syscall
- **EFER.SCE**: System Call Extensions enable bit

### 2. Entry Point (syscall_entry)

Assembly routine that:

- Saves user context (registers, stack)
- Switches to kernel stack
- Calls C dispatcher
- Restores user context
- Returns via SYSRET

### 3. Dispatcher (syscall_handler)

C function that:

- Validates syscall number
- Looks up handler in syscall table
- Calls registered handler
- Returns result to user space

---

## System Call Numbers

System calls are identified by number in RAX register:

```c
#define SYS_EXIT     0   /* Terminate process */
#define SYS_WRITE    1   /* Write to file descriptor */
#define SYS_READ     2   /* Read from file descriptor */
#define SYS_OPEN     3   /* Open file (reserved) */
#define SYS_CLOSE    4   /* Close file (reserved) */
#define SYS_GETPID   5   /* Get process ID */
#define SYS_FORK     6   /* Create child process (reserved) */
#define SYS_EXEC     7   /* Execute program (reserved) */
#define SYS_WAIT     8   /* Wait for child (reserved) */
#define SYS_SBRK     9   /* Adjust heap (reserved) */
#define SYS_SLEEP    10  /* Sleep for duration (reserved) */
#define SYS_YIELD    11  /* Yield CPU to scheduler */
#define SYS_GETTIME  12  /* Get system time (reserved) */
```

### Currently Implemented:

- `SYS_EXIT` (0) - Process termination
- `SYS_WRITE` (1) - Write to stdout/stderr
- `SYS_READ` (2) - Read from stdin
- `SYS_GETPID` (5) - Get process ID
- `SYS_YIELD` (11) - Yield CPU

---

## API Reference

### Kernel-Side Functions

#### syscall_init()

```c
void syscall_init(void);
```

**Description:**  
Initializes the system call subsystem by configuring MSRs and registering
built-in syscall handlers.

**Usage:**

```c
void kmain(void) {
    // ... other initialization ...
    syscall_init();
    // System calls now available
}
```

**What it does:**

1. Clears syscall table
2. Registers built-in handlers
3. Configures MSR_STAR with segment selectors
4. Sets MSR_LSTAR to syscall_entry address
5. Sets MSR_SFMASK to clear IF/DF/TF flags
6. Enables SCE bit in EFER

---

#### syscall_register()

```c
void syscall_register(int num, syscall_handler_t handler);
```

**Description:**  
Registers a custom syscall handler.

**Parameters:**

- `num`: Syscall number (0-12)
- `handler`: Function pointer to handler

**Example:**

```c
static int64_t my_syscall_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                               uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    printf("Custom syscall called with arg1=%llu\n", arg1);
    return 0;
}

void register_my_syscall(void) {
    syscall_register(13, my_syscall_impl);
}
```

---

### User-Space Functions

These functions are used by programs running in Ring 3.

#### sys_exit()

```c
void sys_exit(int status) __attribute__((noreturn));
```

**Description:**  
Terminates the current process with given exit status.

**Parameters:**

- `status`: Exit code (0 = success, non-zero = error)

**Example:**

```c
void user_program(void) {
    sys_write(1, "Exiting...\n", 11);
    sys_exit(0);
}
```

---

#### sys_write()

```c
ssize_t sys_write(int fd, const void *buf, size_t count);
```

**Description:**  
Writes data to a file descriptor.

**Parameters:**

- `fd`: File descriptor (1 = stdout, 2 = stderr)
- `buf`: Pointer to data buffer
- `count`: Number of bytes to write

**Returns:**  
Number of bytes written, or -1 on error

**Example:**

```c
const char *msg = "Hello, World!\n";
ssize_t written = sys_write(1, msg, 14);
if (written < 0) {
    sys_exit(1);
}
```

---

#### sys_read()

```c
ssize_t sys_read(int fd, void *buf, size_t count);
```

**Description:**  
Reads data from a file descriptor.

**Parameters:**

- `fd`: File descriptor (0 = stdin)
- `buf`: Pointer to buffer for data
- `count`: Maximum bytes to read

**Returns:**  
Number of bytes read, or -1 on error

**Example:**

```c
char buffer[256];
ssize_t bytes = sys_read(0, buffer, sizeof(buffer));
if (bytes > 0) {
    sys_write(1, buffer, bytes);
}
```

---

#### sys_getpid()

```c
int sys_getpid(void);
```

**Description:**  
Returns the process ID of the calling process.

**Returns:**  
Process ID (currently always returns 1)

**Example:**

```c
int pid = sys_getpid();
printf("My PID is %d\n", pid);
```

---

#### sys_yield()

```c
void sys_yield(void);
```

**Description:**  
Voluntarily yields the CPU to the scheduler.

**Example:**

```c
while (waiting_for_event) {
    sys_yield();  // Let other processes run
}
```

---

## Usage Examples

### Example 1: Simple User Program

```c
#include <thuban/syscall.h>

void user_main(void) {
    const char *msg = "Hello from user space!\n";
    sys_write(1, msg, 23);
    sys_exit(0);
}
```

### Example 2: Echo Program

```c
#include <thuban/syscall.h>

void user_main(void) {
    char buffer[256];

    sys_write(1, "Enter text: ", 12);

    ssize_t bytes = sys_read(0, buffer, sizeof(buffer));
    if (bytes > 0) {
        sys_write(1, "You typed: ", 11);
        sys_write(1, buffer, bytes);
    }

    sys_exit(0);
}
```

### Example 3: Process Information

```c
#include <thuban/syscall.h>

void user_main(void) {
    int pid = sys_getpid();

    char msg[64];
    int len = sprintf(msg, "Process ID: %d\n", pid);
    sys_write(1, msg, len);

    sys_exit(0);
}
```

### Example 4: Custom Syscall (Kernel Side)

```c
/* Kernel module implementing custom syscall */
static int64_t sys_custom_impl(uint64_t cmd, uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    switch (cmd) {
        case 0:
            printf("Custom command 0 executed\n");
            return 0;
        case 1:
            printf("Custom command 1 with arg=%llu\n", arg1);
            return arg1 * 2;
        default:
            return -1;
    }
}

void module_init(void) {
    syscall_register(13, sys_custom_impl);
}
```

---

## Implementation Details

### Calling Convention

System calls follow the x86_64 System V ABI with modifications:

**Arguments (User → Kernel):**

```
RAX = syscall number
RDI = arg1
RSI = arg2
RDX = arg3
R10 = arg4 (note: not RCX, which is used by SYSCALL)
R8  = arg5
R9  = arg6
```

**Return Value (Kernel → User):**

```
RAX = return value
```

**Preserved by SYSCALL instruction:**

```
RCX = User RIP (return address)
R11 = User RFLAGS
```

### Entry Sequence

1. **User space executes:** `syscall` instruction
2. **CPU atomically:**
   - Saves RIP to RCX
   - Saves RFLAGS to R11
   - Loads RIP from MSR_LSTAR
   - Loads CS from MSR_STAR[47:32]
   - Clears RFLAGS bits per MSR_SFMASK
   - Transitions to Ring 0

3. **syscall_entry:**
   - Saves user RSP
   - Switches to kernel stack
   - Pushes all registers
   - Calls syscall_handler()

4. **syscall_handler:**
   - Validates syscall number
   - Calls registered handler
   - Returns result

5. **syscall_entry return:**
   - Restores all registers
   - Puts return value in RAX
   - Executes SYSRET

6. **CPU atomically:**
   - Loads RIP from RCX
   - Loads RFLAGS from R11
   - Loads CS from MSR_STAR[63:48] + 16
   - Transitions to Ring 3

### Stack Layout During Syscall

```
Kernel Stack (after syscall_entry saves context):

[Top]
R15
R14
R13
R12
R11
R10
R9
R8
RBP
RDI
RSI
RDX
RCX
RBX
RAX
RIP (from RCX)
CS
RFLAGS (from R11)
RSP (user)
SS
[Bottom]
```

### Handler Function Signature

```c
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5, uint64_t arg6);
```

All handlers must:

- Accept 6 uint64_t arguments
- Return int64_t (or cast appropriately)
- Be reentrant and thread-safe (future)
- Handle invalid arguments gracefully

### Error Handling

System calls return -1 on error. Currently:

- Invalid syscall number → returns -1
- Invalid file descriptor → returns -1
- NULL buffer pointer → returns -1
- Unimplemented syscall → returns -1

---

## Integration

### Adding System Calls to Kernel

**Step 1:** Define syscall number in `syscall.h`:

```c
#define SYS_MYNEWCALL 13
```

**Step 2:** Implement handler in `syscall.c`:

```c
static int64_t sys_mynewcall_impl(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                  uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    // Implementation here
    return 0;
}
```

**Step 3:** Register in `syscall_init()`:

```c
void syscall_init(void) {
    // ... existing registrations ...
    syscall_register(SYS_MYNEWCALL, sys_mynewcall_impl);
}
```

**Step 4:** Add user-space wrapper in `syscall.h`:

```c
static inline int64_t sys_mynewcall(uint64_t arg1) {
    return syscall(SYS_MYNEWCALL, arg1, 0, 0, 0, 0);
}
```

### Using in User Programs

**Include header:**

```c
#include <thuban/syscall.h>
```

**Call wrapper functions:**

```c
sys_write(1, "Hello\n", 6);
int pid = sys_getpid();
sys_exit(0);
```

### Testing from Kernel Mode

For testing without Ring 3 transition:

```c
extern int64_t syscall_handler(uint64_t num, uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4, uint64_t arg5);

void test_syscalls(void) {
    // Test SYS_WRITE
    const char *msg = "Test message\n";
    int64_t result = syscall_handler(SYS_WRITE, 1, (uint64_t)msg, 13, 0, 0);

    // Test SYS_GETPID
    result = syscall_handler(SYS_GETPID, 0, 0, 0, 0, 0);
    printf("PID: %lld\n", result);
}
```

---

## Register State

### On Entry to Syscall Handler

```
RAX = syscall number (consumed)
RDI = arg1 (passed to handler)
RSI = arg2 (passed to handler)
RDX = arg3 (passed to handler)
R10 = arg4 (passed to handler)
R8  = arg5 (passed to handler)
R9  = arg6 (passed to handler)

RCX = user RIP (saved by CPU)
R11 = user RFLAGS (saved by CPU)
RSP = kernel stack (switched by syscall_entry)

All other registers preserved
```

### On Return from Syscall

```
RAX = return value
RCX = user RIP (restored)
R11 = user RFLAGS (restored)
RSP = user stack (restored)

All other registers restored to original values
```

---

## Security Considerations

### Current Implementation

- **No permission checks**: All syscalls available to all processes
- **No address validation**: User pointers not validated
- **No resource limits**: No quotas or rate limiting

### Ring Protection

- User programs run in Ring 3 (CPL=3)
- Kernel runs in Ring 0 (CPL=0)
- SYSCALL instruction enforces privilege transition
- User programs cannot directly access kernel memory

### Interrupt Safety

- Interrupts disabled during syscall_entry
- Re-enabled in handler if needed
- Prevents reentrancy issues

---

## Performance

### Syscall Overhead

Approximate cycle counts (on modern x86_64):

- SYSCALL instruction: ~50-100 cycles
- Context save/restore: ~50-100 cycles
- Handler dispatch: ~10-20 cycles
- SYSRET instruction: ~50-100 cycles
- **Total: ~160-320 cycles per syscall**

### Optimization

- MSRs configured once at boot
- Handler lookup is direct array access O(1)
- Minimal stack operations
- No memory allocation during syscall

---

## Debugging

### Enable Syscall Logging

Add to handlers for debugging:

```c
printf("[SYSCALL] %s called with args: %llu, %llu, %llu\n",
       "write", arg1, arg2, arg3);
```

### Common Issues

**Symptom:** Triple fault on syscall  
**Cause:** MSRs not configured  
**Fix:** Ensure `syscall_init()` called before Ring 3 transition

**Symptom:** Page fault in user code  
**Cause:** User pages not mapped with USER flag  
**Fix:** Map user code/data with `PAGING_USER` flag

**Symptom:** Invalid opcode  
**Cause:** SYSCALL not supported (old CPU) or SCE not enabled  
**Fix:** Check CPUID and enable EFER.SCE

---

## References

### Related Subsystems

- **GDT**: Segment descriptors for Ring 0/3 (kernel/gdt.c)
- **Paging**: User/kernel page permissions (mm/paging.c)
- **Interrupts**: Alternative kernel entry mechanism (kernel/interrupts.c)

### External Documentation

- AMD64 Architecture Programmer's Manual Vol 2 (System Programming)
- Intel 64 and IA-32 Architectures Software Developer's Manual Vol 3
- System V ABI AMD64 Supplement

---

## See Also

- `panic(9)` - Kernel panic system
- `process(9)` - Process management (future)
- `scheduler(9)` - Task scheduling (future)

---

**Maintainer:** Trollycat  
**License:** MIT  
**Last Updated:** January 2026
