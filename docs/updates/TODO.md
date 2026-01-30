# Thuban OS - Development Roadmap

## Version 0.3.0

**Goal:** Basic disk I/O and filesystem support

### Virtual Filesystem (VFS)

- VFS architecture
- inode structure
- dentry (directory entry) cache
- file structure
- superblock structure
- File operations
- open(), close(), read(), write()
- lseek(), stat()
- Directory operations
- readdir(), mkdir(), rmdir()
- Path resolution
- Absolute paths (/path/to/file)
- Relative paths (./file, ../file)
- Symbolic links (optional)

### Filesystem Implementation

- **FAT32:** (Real disk filesystem)
- FAT32 structures (boot sector, FAT, clusters)
- Read files from FAT32 partition
- Write support

### Extended Syscalls

- File I/O syscalls
- sys_open (3)
- sys_close (4)
- sys_lseek (8)
- sys_stat (4)
- sys_fstat (5)
- Directory syscalls
- sys_mkdir (83)
- sys_rmdir (84)
- sys_getdents (78)

### Testing & Integration

- Shell commands for filesystem
- ls - list directory
- cat - display file contents
- mkdir - create directory
- rm - remove file
- Test file creation/deletion
- Test directory traversal

---

## Version 0.3.1

**Goal:** Load and execute user programs

### ELF Loader

- ELF header parsing
- Identify ELF magic number
- Parse program headers
- Parse section headers
- Load program into memory
- Allocate memory for segments
- Load code/data sections
- Set up BSS (zero-initialized data)
- Relocations (if needed)
- Set up user stack
- Jump to entry point

### Process Management

- Process Control Block (PCB/task_struct)
- Process ID (PID)
- Process state (running, ready, blocked, zombie)
- Register context (for context switching)
- Memory map (page tables)
- Open file table
- Parent/child relationships
- Process creation
- sys_fork() - create child process
- sys_exec() - replace process image
- Copy-on-write (COW) for fork - optional
- Process termination
- sys_exit() - terminate process
- sys_wait() / sys_waitpid() - wait for child
- Zombie process handling
- Resource cleanup

### Basic Scheduler

- Round-robin scheduler
- Process queue (ready queue)
- Time slice (quantum)
- Timer interrupt for preemption
- Context switching
- Save current process state
- Load next process state
- Switch page tables
- Process states
- Running, ready, blocked, zombie

### Enhanced Ring Mode

- User process isolation
- Separate page tables per process
- User heap/stack
- Copy user data safely (copy_from_user, copy_to_user)
- Process syscalls
- sys_fork (2)
- sys_execve (59)
- sys_wait (61)
- sys_getpid (39)
- sys_kill (62)

### Testing

- Load and execute simple user program
- Test fork/exec
- Test multiple processes running
- Test process termination

---

## Version 0.3.2

**Goal:** SMP support and kernel threads

### CPU Detection & Initialization

- ACPI parsing
- Find RSDP (Root System Description Pointer)
- Parse RSDT/XSDT
- Parse MADT (Multiple APIC Description Table)
- Enumerate CPUs
- LAPIC (Local APIC) setup
- Enable LAPIC per CPU
- Configure timer
- IPI (Inter-Processor Interrupt) support
- IOAPIC (I/O APIC) setup
- Route IRQs through IOAPIC
- Replace PIC with APIC
- SMP boot protocol
- Bootstrap processor (BSP) initialization
- Application processor (AP) startup
- Trampoline code for APs

### Per-CPU Data

- Per-CPU variable infrastructure
- Per-CPU kernel stacks
- Per-CPU IDT/GDT
- CPU-local scheduler queues

### Real Spinlocks (Atomic)

- Atomic operations
- LOCK XCHG instruction
- LOCK CMPXCHG instruction
- Memory barriers
- Ticket spinlocks
- Fair spinlock implementation
- spin_lock(), spin_unlock()
- spin_trylock()
- Read-write locks
- Multiple readers, single writer
- Update all existing spinlocks

### Threading

- Kernel thread support
- kthread_create()
- kthread_stop()
- Thread synchronization
- Mutexes
- Semaphores
- Condition variables
- User threads (pthreads-style)
- clone() syscall
- Thread-local storage (TLS)

### Enhanced Scheduler

- Multi-core scheduling
- Load balancing across CPUs
- CPU affinity
- Priority scheduling
- Nice values
- Real-time priorities

---

## Version 0.3.3

**Goal:** Loadable kernel modules (like Linux .ko files)

### Module Infrastructure

- Module structure
- Module header
- Symbol table export/import
- Module dependencies
- License information
- Module states
- Loading, loaded, unloading
- Reference counting

### Module Loading

- insmod functionality
- Load .ko file from filesystem
- Parse ELF (kernel module format)
- Allocate kernel memory for module
- Perform relocations
- Link with kernel symbols
- Call module_init()
- Automatic module loading
- Based on hardware detection
- Module aliases

### Module Unloading

- rmmod functionality
- Check if module is in use
- Call module_exit()
- Unlink from kernel
- Free memory
- Force unload (dangerous, optional)

### Module Management

- lsmod - list loaded modules
- modprobe - smart module loading
- modinfo - show module information
- Module dependencies (modules.dep)

### Driver Framework Integration

- Convert static drivers to modules
- VGA driver as module
- Keyboard driver as module
- Future drivers as modules

---

## Version 0.3.4

**Goal:** Automatic hardware discovery and driver loading

### PCI Bus Support

- PCI configuration space access
- I/O ports (0xCF8, 0xCFC)
- Memory-mapped (PCIe ECAM)
- PCI enumeration
- Scan all buses (0-255)
- Scan all devices (0-31 per bus)
- Scan all functions (0-7 per device)
- PCI device information
- Vendor ID, Device ID
- Class code, subclass
- BAR (Base Address Register) mapping
- Interrupt routing
- PCI vendor/device database
- Map IDs to human-readable names

### Device Tree

- Device tree structure
- Bus hierarchy
- Device nodes
- Resource allocation
- Device matching
- Match devices to drivers
- Automatic driver loading
- Hot-plug support

### Additional Storage Drivers

- SATA
- ACHI
- NVMe driver
- NVMe admin queue
- I/O queues
- NVMe commands

---

## Version 0.3.5

**Goal:** Support multiple filesystem types

### ext2 Filesystem

- ext2 superblock parsing
- Inode table
- Block groups
- Directory entries
- File read/write

### ext4

- Extents
- Journal support
- Advanced features

### NTFS (Read-only)

- NTFS boot sector
- MFT (Master File Table)
- File records
- Attribute records
- Read-only support initially

---
