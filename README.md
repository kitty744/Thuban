# Thuban

A modern x86_64 operating system built from scratch with comprehensive hardware compatibility and a retro GUI aesthetic.

[Join our Discord server!](https://discord.gg/JR4znNRFw8)

## Overview

Thuban is an operating system designed to run on both old and new hardware.

## Build Requirements

### Toolchain

- **x86_64-elf-gcc**: Cross-compiler for x86_64
- **nasm**: Assembly compiler
- **x86_64-elf-ld**: Linker
- **grub-mkrescue**: ISO creation
- **qemu-system-x86_64**: Emulation/testing
- **kconfig-frontends**: Kernel configuration frontend
- **coccinelle**: Debugging/Safety.

## Building and Running

### Quick Start

```bash
# 1. Install dependencies
make install

# 2. Build and run with configured settings
make run

# 3. Build only (no QEMU)
make all

# 4. Clean build artifacts
make clean
```

### Build Process

1. **Assembly**: NASM compiles boot.s with elf32 format
2. **Compilation**: GCC compiles all C files with kernel flags
3. **Linking**: LD creates the ELF kernel with proper memory layout
4. **ISO Creation**: GRUB creates bootable ISO with multiboot2 support
5. **Testing**: QEMU launches the OS with CD-ROM boot

## License

This project is open source. See LICENSE file for details.

## Acknowledgments

- Inspired by Linux kernel architecture and design patterns
- Built with modern OS development best practices
- Compatible with existing hardware and software standards

---

**Thuban** - Building the future of retro computing with modern technology.
