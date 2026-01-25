#!/bin/bash

# This script Install's all required dependencies for building.
# You will likely run this before booting the operating system.

# Exit upon error's
set -e

echo "Installing Thuban OS dependencies..."

# check if running on debian/ubuntu based system
if command -v apt-get &> /dev/null; then
    echo "Detected Debian/Ubuntu system"
    sudo apt-get update
    sudo apt-get install -y build-essential nasm grub-pc-bin grub-common xorriso qemu-system-x86
    
# check if running on arch based system  
elif command -v pacman &> /dev/null; then
    echo "Detected Arch-based system"
    sudo pacman -S --needed base-devel nasm grub libisoburn qemu-system-x86
    
# check if running on fedora/rhel based system
elif command -v dnf &> /dev/null; then
    echo "Detected Fedora/RHEL system"
    sudo dnf install -y gcc make nasm grub2-tools xorriso qemu-system-x86
    
else
    echo "Unsupported package manager"
    echo "Please manually install: nasm, grub-mkrescue, xorriso, qemu-system-x86_64"
    exit 1
fi

# check if cross compiler exists
if ! command -v x86_64-elf-gcc &> /dev/null; then
    echo ""
    echo "WARNING: x86_64-elf-gcc cross compiler not found"
    echo "You need to build the cross compiler manually"
    echo "See: https://wiki.osdev.org/GCC_Cross-Compiler"
    echo ""
    echo "Quick guide:"
    echo "  export PREFIX=/usr/local/cross"
    echo "  export TARGET=x86_64-elf"
    echo "  export PATH=\$PREFIX/bin:\$PATH"
    echo "  Then build binutils and gcc for x86_64-elf target"
    exit 1
fi

echo ""
echo "Dependencies installed successfully"
echo "You can now run: make run"