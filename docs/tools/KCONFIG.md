# Kconfig - Kernel Configuration System

## Overview

Kconfig is the configuration system used by Thuban OS (borrowed from Linux). It provides a menu-driven interface for configuring kernel features, drivers, and build options. This allows you to enable/disable features at compile-time rather than runtime.

## Table of Contents

1. [What is Kconfig?](#what-is-kconfig)
2. [Getting Started](#getting-started)
3. [Configuration Syntax](#configuration-syntax)
4. [Menu Structure](#menu-structure)
5. [Configuration Options](#configuration-options)
6. [Dependencies](#dependencies)
7. [Default Values](#default-values)
8. [Advanced Features](#advanced-features)
9. [Best Practices](#best-practices)
10. [Examples](#examples)

---

## What is Kconfig?

Kconfig is a language used to define configuration options for the kernel. It provides:

- **Menu-based configuration** via `make menuconfig`
- **Dependency management** between options
- **Automatic .config file generation**
- **Conditional compilation** via preprocessor macros

### Why Use Kconfig?

**Benefits:**

- **Modularity** - Enable only features you need
- **Smaller binaries** - Exclude unused code
- **Clear dependencies** - Options automatically enable/disable based on requirements
- **User-friendly** - Menu interface instead of editing headers
- **Maintainability** - Centralized configuration

**Example:**

```
# Instead of editing headers:
#define ENABLE_VGA_DRIVER 1
#define ENABLE_PS2_KEYBOARD 1

# Use Kconfig:
make menuconfig
  [*] VGA Text Mode Driver
  [*] PS/2 Keyboard Driver
```

---

## Getting Started

### Installing Tools

**Ubuntu/Debian:**

```bash
sudo apt-get install build-essential libncurses-dev bison flex
```

**Arch Linux:**

```bash
sudo pacman -S base-devel ncurses bison flex
```

**macOS:**

```bash
brew install ncurses bison flex
```

### Basic Usage

**1. Configure kernel:**

```bash
make menuconfig
```

**2. Navigate menus:**

- **Arrow keys** - Move between options
- **Enter** - Enter submenu or toggle option
- **Space** - Toggle option (Y/N/M)
- **?** - Show help for selected option
- **/** - Search for options
- **Esc** - Go back / Exit

**3. Save configuration:**

- Select "Save" from main menu
- Press Enter to save to `.config`

**4. Build kernel:**

```bash
make clean
make
```

### Configuration Files

| File          | Purpose                                    |
| ------------- | ------------------------------------------ |
| `Kconfig`     | Root configuration file                    |
| `.config`     | Generated configuration (selected options) |
| `.config.old` | Backup of previous configuration           |

---

## Configuration Syntax

### Basic Structure

```kconfig
config OPTION_NAME
    bool "Human-readable description"
    default y
    help
      Detailed help text explaining what this option does.
      Can span multiple lines.
```

### Entry Types

| Type       | Description        | Values                           |
| ---------- | ------------------ | -------------------------------- |
| `bool`     | Boolean option     | y (yes), n (no)                  |
| `tristate` | Three-state option | y (built-in), m (module), n (no) |
| `int`      | Integer value      | Any integer                      |
| `hex`      | Hexadecimal value  | 0x prefix                        |
| `string`   | String value       | Quoted string                    |

**Examples:**

```kconfig
config DEBUG
    bool "Enable debugging"

config MAX_CPUS
    int "Maximum number of CPUs"
    default 8

config KERNEL_BASE
    hex "Kernel base address"
    default 0xFFFFFFFF80000000

config KERNEL_VERSION
    string "Kernel version string"
    default "0.2"
```

---

## Menu Structure

### Creating Menus

```kconfig
menu "Device Drivers"

config VGA_DRIVER
    bool "VGA Text Mode Driver"

config PS2_KEYBOARD
    bool "PS/2 Keyboard Driver"

endmenu
```

### Submenus

```kconfig
menu "Memory Management"

config PAGING
    bool "Enable paging"
    default y

menu "Memory Allocators"
    depends on PAGING

config HEAP
    bool "Heap allocator"

config SLAB
    bool "Slab allocator"

endmenu

endmenu
```

### Menu Organization

**Root Kconfig structure:**

```kconfig
mainmenu "Thuban OS Configuration"

menu "General Setup"
    source "kernel/Kconfig"
endmenu

menu "Processor Features"
    source "arch/x86_64/Kconfig"
endmenu

menu "Device Drivers"
    source "drivers/Kconfig"
endmenu
```

---

## Configuration Options

### Boolean Options

```kconfig
config ENABLE_FEATURE
    bool "Enable this feature"
    default y
    help
      This enables the feature. Say Y if you want it.
```

**Generated macro:**

```c
#define CONFIG_ENABLE_FEATURE 1
// or undefined if disabled
```

**Usage in code:**

```c
#ifdef CONFIG_ENABLE_FEATURE
    // Feature-specific code
#endif
```

### Integer Options

```kconfig
config BUFFER_SIZE
    int "Buffer size in bytes"
    range 512 65536
    default 4096
    help
      Size of the internal buffer.
```

**Generated macro:**

```c
#define CONFIG_BUFFER_SIZE 4096
```

**Usage in code:**

```c
char buffer[CONFIG_BUFFER_SIZE];
```

### String Options

```kconfig
config VERSION_STRING
    string "Kernel version"
    default "0.2-dev"
```

**Generated macro:**

```c
#define CONFIG_VERSION_STRING "0.2-dev"
```

**Usage in code:**

```c
printf("Kernel version: %s\n", CONFIG_VERSION_STRING);
```

---

## Dependencies

### Simple Dependencies

```kconfig
config USB_DRIVER
    bool "USB Driver"
    depends on PCI
```

**Effect:** USB_DRIVER only appears if PCI is enabled.

### Multiple Dependencies

```kconfig
config ADVANCED_FEATURE
    bool "Advanced Feature"
    depends on FEATURE_A && FEATURE_B
```

**Operators:**

- `&&` - AND
- `||` - OR
- `!` - NOT

### Reverse Dependencies (select)

```kconfig
config FEATURE_A
    bool "Feature A"
    select FEATURE_B
    select FEATURE_C
```

**Effect:** Enabling FEATURE_A automatically enables FEATURE_B and FEATURE_C.

**Example:**

```kconfig
config VGA_DRIVER
    bool "VGA Driver"
    select VIDEO_SUBSYSTEM
    help
      VGA text mode driver. Automatically enables video subsystem.
```

### Implications (imply)

```kconfig
config NETWORKING
    bool "Networking support"
    imply ETHERNET_DRIVER
```

**Effect:** Enabling NETWORKING suggests enabling ETHERNET_DRIVER (but doesn't force it).

---

## Default Values

### Static Defaults

```kconfig
config FEATURE
    bool "Enable feature"
    default y
```

### Conditional Defaults

```kconfig
config DEBUG_LEVEL
    int "Debug level"
    default 0 if !DEBUG
    default 2 if DEBUG
```

### Architecture-Specific Defaults

```kconfig
config PAGE_SIZE
    int "Page size"
    default 4096 if X86_64
    default 16384 if ARM64
```

---

## Advanced Features

### Choice Groups

```kconfig
choice
    prompt "Scheduler Type"
    default SCHED_SIMPLE

config SCHED_SIMPLE
    bool "Simple Round-Robin"

config SCHED_PRIORITY
    bool "Priority-Based"

config SCHED_CFS
    bool "Completely Fair Scheduler"

endchoice
```

**Effect:** Only one option can be selected at a time.

### Visibility Conditions

```kconfig
config ADVANCED_OPTIONS
    bool "Show advanced options"

config EXPERT_FEATURE
    bool "Expert feature" if ADVANCED_OPTIONS
    default n
```

**Effect:** EXPERT_FEATURE only visible if ADVANCED_OPTIONS is enabled.

### Ranges

```kconfig
config NUM_BUFFERS
    int "Number of buffers"
    range 1 256
    default 16
```

**Effect:** Value must be between 1 and 256.

---

## Best Practices

### 1. Clear Naming

**Good:**

```kconfig
config VGA_TEXT_MODE
    bool "VGA Text Mode Driver"
```

**Bad:**

```kconfig
config VGA_DRV
    bool "VGA"
```

### 2. Comprehensive Help Text

**Good:**

```kconfig
config HEAP_DEBUG
    bool "Heap debugging"
    help
      Enable heap allocator debugging. This adds checks for:
      - Double frees
      - Memory corruption
      - Leaks

      Adds ~2KB to kernel size and slight performance overhead.
      Recommended for development builds only.
```

**Bad:**

```kconfig
config HEAP_DEBUG
    bool "Heap debugging"
```

### 3. Logical Dependencies

```kconfig
config NETWORK_DRIVER
    bool "Network driver"
    depends on NETWORKING
    select DMA_ENGINE
```

### 4. Sensible Defaults

```kconfig
config VGA_DRIVER
    bool "VGA Driver"
    default y  # Most systems have VGA

config EXPERIMENTAL_FS
    bool "Experimental filesystem"
    default n  # Experimental, default off
```

### 5. Group Related Options

```kconfig
menu "Memory Management"

config HEAP
    bool "Heap allocator"

config HEAP_DEBUG
    bool "Heap debugging"
    depends on HEAP

config HEAP_STATS
    bool "Heap statistics"
    depends on HEAP

endmenu
```

---

## Examples

### Example 1: Simple Driver Configuration

```kconfig
# drivers/video/Kconfig

menu "Video Drivers"

config VIDEO_SUBSYSTEM
    bool "Video support"
    help
      Enable video subsystem support.

config VGA_DRIVER
    bool "VGA Text Mode Driver"
    depends on VIDEO_SUBSYSTEM
    default y
    help
      Driver for VGA text mode (80x25 characters).
      Required for console output.

config VGA_RESOLUTION
    int "VGA columns"
    depends on VGA_DRIVER
    range 80 132
    default 80
    help
      Number of columns in VGA text mode.
      Standard is 80, some cards support 132.

endmenu
```

### Example 2: Architecture-Specific Config

```kconfig
# arch/x86_64/Kconfig

config X86_64
    def_bool y
    select HAVE_PAGING
    select HAVE_INTERRUPTS

config KERNEL_BASE
    hex "Kernel virtual base address"
    default 0xFFFFFFFF80000000
    help
      Virtual address where kernel is loaded.
      Default is -2GB (higher half kernel).

config MAX_CPUS
    int "Maximum number of CPUs"
    range 1 256
    default 8
    help
      Maximum number of CPUs supported.
      Affects data structure sizes.
```

### Example 3: Debugging Options

```kconfig
# kernel/Kconfig

menu "Debugging"

config DEBUG
    bool "Enable debugging"
    default n
    help
      Enable debugging features. Increases kernel size
      and reduces performance. Recommended for development.

config DEBUG_VERBOSE
    bool "Verbose debug output"
    depends on DEBUG
    default n
    help
      Enable verbose debug messages. Very chatty!

config PANIC_ON_OOPS
    bool "Panic on oops"
    default y if !DEBUG
    default n if DEBUG
    help
      When enabled, kernel panics on errors.
      When disabled, attempts recovery (dangerous).

endmenu
```

### Example 4: Feature Selection

```kconfig
choice
    prompt "Memory Allocator"
    default HEAP_ALLOCATOR

config HEAP_ALLOCATOR
    bool "Simple heap allocator"
    help
      Basic malloc/free implementation.
      Small and simple.

config SLAB_ALLOCATOR
    bool "Slab allocator"
    help
      Cache-friendly slab allocator.
      Better performance, more complex.

config BUDDY_ALLOCATOR
    bool "Buddy allocator"
    help
      Power-of-two buddy system.
      Good fragmentation handling.

endchoice
```

---

## Using Configuration in Code

### Checking if Feature is Enabled

```c
#ifdef CONFIG_DEBUG
    printf("[DEBUG] Entering function %s\n", __func__);
#endif

#ifdef CONFIG_VGA_DRIVER
    vga_init();
#endif
```

### Using Configuration Values

```c
#define BUFFER_SIZE CONFIG_BUFFER_SIZE
char buffer[BUFFER_SIZE];

printf("Kernel version: %s\n", CONFIG_VERSION_STRING);

#if CONFIG_MAX_CPUS > 64
    #error "Too many CPUs configured"
#endif
```

### Conditional Compilation

```c
void init_drivers(void) {
#ifdef CONFIG_VGA_DRIVER
    vga_init();
#endif

#ifdef CONFIG_PS2_KEYBOARD
    keyboard_init();
#endif

#ifdef CONFIG_NETWORK_DRIVER
    network_init();
#endif
}
```

---

## Makefile Integration

### Conditional Object Files

```makefile
# Makefile
obj-$(CONFIG_VGA_DRIVER) += drivers/video/vga.o
obj-$(CONFIG_PS2_KEYBOARD) += drivers/input/ps2.o
obj-$(CONFIG_HEAP_ALLOCATOR) += mm/heap.o
```

### Using Config Values

```makefile
CFLAGS-$(CONFIG_DEBUG) += -g -DDEBUG
CFLAGS-$(CONFIG_OPTIMIZE) += -O2

KERNEL_BASE := $(CONFIG_KERNEL_BASE)
```

---

## Troubleshooting

### "Recursive dependency detected"

**Problem:**

```kconfig
config A
    select B

config B
    select A
```

**Solution:** Remove circular dependency.

### "undefined symbol"

**Problem:** Referenced config option doesn't exist.

**Solution:** Check spelling and ensure option is defined.

### Changes not taking effect

**Problem:** `.config` not regenerated.

**Solution:**

```bash
make clean
make menuconfig
make
```

### Option not visible

**Problem:** Dependencies not met.

**Solution:** Enable required dependencies first, or use search (`/` key) to find why option is hidden.

---

## Advanced Topics

### Kconfig Macros

```kconfig
config $(shell uname -m)
    def_bool y
```

### Including External Files

```kconfig
source "drivers/Kconfig"
source "arch/*/Kconfig"
```

### Environment Variables

```kconfig
config CUSTOM_OPTION
    string "Custom setting"
    default "$(CUSTOM_VALUE)"
```

---

## Quick Reference

### Common Patterns

**Enable by default:**

```kconfig
config FEATURE
    bool "Feature"
    default y
```

**Require another option:**

```kconfig
config FEATURE
    bool "Feature"
    depends on OTHER_FEATURE
```

**Auto-enable dependencies:**

```kconfig
config FEATURE
    bool "Feature"
    select DEPENDENCY
```

**Mutually exclusive options:**

```kconfig
choice
    prompt "Select one"
config OPTION_A
    bool "A"
config OPTION_B
    bool "B"
endchoice
```

---

## See Also

- [Linux Kconfig Documentation](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
- [Kconfig Macro Language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-macro-language.html)

---

**Maintainer:** Trollycat  
**License:** MIT  
**Last Updated:** January 2026
