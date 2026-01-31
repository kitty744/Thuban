CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld

CFLAGS = -m64 -nostdlib -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -I$(PWD)/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld -z max-page-size=0x1000

BUILD_DIR = build
KERNEL_BIN = $(BUILD_DIR)/thuban.bin
KERNEL_ISO = $(BUILD_DIR)/thuban.iso

# Disk image settings (read from .config if exists)
-include .config

DISK_IMAGE ?= disk.img
DISK_SIZE ?= 100M
DISK_FORMAT ?= raw

objs-y := 

subdirs-y := arch/ kernel/ mm/ drivers/ lib/ fs/

include $(patsubst %,%/Makefile,$(subdirs-y))

KERNEL_OBJS = $(addprefix $(BUILD_DIR)/,$(objs-y))

all: $(KERNEL_ISO)

# Create disk image if it doesn't exist
$(DISK_IMAGE):
	@qemu-img create -f $(DISK_FORMAT) $(DISK_IMAGE) $(DISK_SIZE)
	@mkfs.vfat -F 32 $(DISK_IMAGE)

$(KERNEL_ISO): $(KERNEL_BIN) $(DISK_IMAGE)
	@mkdir -p $(BUILD_DIR)/isofiles/boot/grub
	cp $(KERNEL_BIN) $(BUILD_DIR)/isofiles/boot/thuban.bin
	echo 'set timeout=0' > $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	echo 'set default=0' >> $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	echo 'menuentry "Thuban OS" {' >> $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	echo '    multiboot2 /boot/thuban.bin' >> $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	echo '    boot' >> $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	echo '}' >> $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	grub-mkrescue -o $(KERNEL_ISO) $(BUILD_DIR)/isofiles

$(KERNEL_BIN): $(KERNEL_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

install:
	@chmod +x ./scripts/install.sh
	./scripts/install.sh

run:
	@chmod +x ./scripts/run.sh
	./scripts/run.sh

menuconfig:
	kconfig-mconf Kconfig

clean:
	rm -rf $(BUILD_DIR)
.PHONY: all clean install run menuconfig