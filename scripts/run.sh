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
DISK_FMT=$(echo ${CONFIG_DISK_FORMAT:-raw} | tr -d '"')

# Disk image should be created by Makefile
# Just add it to QEMU command
Q_STORAGE="-drive file=$DISK_IMG,format=$DISK_FMT,index=0,media=disk"

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
    -boot d \
    -cdrom build/thuban.iso \
    $Q_STORAGE \
    $Q_AUDIO \
    $EXTRA_ARGS

# Cleanup after closing QEMU
make clean
clear