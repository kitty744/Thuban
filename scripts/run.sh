#!/bin/bash

# Main script to build and boot the operating system using QEMU setting's defined in .config

# --- 1. MAP KCONFIG TO QEMU ---
# If .config exists, Make will have exported these. If not, we use defaults.
Q_MEM=$(echo ${CONFIG_MEM_SIZE:-2G} | tr -d '"')
Q_SMP=$(echo ${CONFIG_CPU_CORES:-5} | tr -d '"')
Q_ARCH=$(echo ${CONFIG_MACHINE_TYPE:-pc} | tr -d '"')
Q_CPU=$(echo ${CONFIG_CPU_TYPE:-qemu64} | tr -d '"')
Q_DEBUG=$(echo ${CONFIG_DEBUG_FLAGS:-guest_errors} | tr -d '"')

# Handle VGA Choice logic
Q_VGA="std"
if [ "$CONFIG_VGA_NONE" = "y" ]; then Q_VGA="none"; fi
if [ "$CONFIG_VGA_VIRTIO" = "y" ]; then Q_VGA="virtio"; fi

# Handle Audio logic
Q_AUDIO=""
if [ "$CONFIG_AUDIO_ENABLED" = "y" ]; then
    # Note: Modern QEMU prefers -audiodev. pcspk-audiodev connects the speaker to the backend.
    Q_AUDIO="-machine $Q_ARCH,pcspk-audiodev=audio0 -audiodev sdl,id=audio0 -soundhw ${CONFIG_SOUNDHW:-pcspk}"
fi

# Handle Storage logic
Q_STORAGE=""
DISK_IMG=$(echo ${CONFIG_DISK_IMAGE:-disk.img} | tr -d '"')
DISK_SIZE=$(echo ${CONFIG_DISK_SIZE:-100M} | tr -d '"')
DISK_FMT=$(echo ${CONFIG_DISK_FORMAT:-raw} | tr -d '"')

# Create disk image if it doesn't exist
if [ ! -f "$DISK_IMG" ]; then
    qemu-img create -f $DISK_FMT "$DISK_IMG" $DISK_SIZE
    if [ $? -ne 0 ]; then
        exit 1
    fi
fi

# Add disk to QEMU command
# Using -hda for IDE/ATA compatibility (primary master)
Q_STORAGE="-hda $DISK_IMG"

# --- 2. BUILD & RUN ---
make clean && make all || exit 1
clear

qemu-system-x86_64 \
    -m $Q_MEM \
    -smp $Q_SMP \
    -machine $Q_ARCH \
    -cpu $Q_CPU \
    -vga $Q_VGA \
    -d $Q_DEBUG \
    -serial stdio \
    -cdrom build/thuban.iso \
    $Q_STORAGE \
    $Q_AUDIO \
    $EXTRA_ARGS

# Cleanup after closing QEMU
make clean
clear