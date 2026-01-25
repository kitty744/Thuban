#!/bin/bash

set -e

ISO="build/thuban.iso"

if [ ! -f "$ISO" ]; then
    echo "Error: $ISO not found"
    echo "Run 'make all' first"
    exit 1
fi

# hardcoded QEMU settings for now
# will be replaced with Kconfig later
qemu-system-x86_64 \
    -cdrom "$ISO" \
    -m 512M \
    -serial stdio \

make clean
clear
clear