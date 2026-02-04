HOSTCC ?= gcc
NASM   ?= nasm
LD     ?= ld

KERNEL_CFLAGS := -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
                 -Wall -Wextra -O2 -I.
KERNEL_LDFLAGS := -nostdlib

ISO_DIR   := iso_root
BUILD_DIR := build

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO_KERNEL := $(ISO_DIR)/boot/kernel.elf

ISO_IMAGE := $(BUILD_DIR)/my-hobby-os.iso

.PHONY: all clean run iso
all: $(ISO_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

check-limine:
	@test -f "$(LIMINE_DIR)/limine-cd.bin"     || (echo "Missing limine/limine-cd.bin" && exit 1)
	@test -f "$(LIMINE_DIR)/limine-cd-efi.bin" || (echo "Missing limine/limine-cd-efi.bin" && exit 1)
	@test -f "$(LIMINE_DIR)/limine-bios.sys"   || (echo "Missing limine/limine-bios.sys" && exit 1)
	@test -f "$(LIMINE_DIR)/BOOTX64.EFI"       || (echo "Missing limine/BOOTX64.EFI" && exit 1)

$(BUILD_DIR)/entry.o: kernel/entry.asm | $(BUILD_DIR)
	$(NASM) -f elf64 $< -o $@

$(BUILD_DIR)/interrupts.o: src/interrupts.asm | $(BUILD_DIR)
	$(NASM) -f elf64 $< -o $@

$(BUILD_DIR)/main.o: src/main.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/gdt.o: src/gdt.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/idt.o: src/idt.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/pic.o: src/pic.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/pmm.o: src/pmm.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/vmm.o: src/vmm.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/keyboard.o: src/keyboard.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/shell.o: src/shell.c | $(BUILD_DIR)
	$(HOSTCC) $(KERNEL_CFLAGS) -Isrc -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR)/entry.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/main.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/vmm.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/shell.o kernel/linker.ld
	$(LD) $(KERNEL_LDFLAGS) -T kernel/linker.ld -o $@ $(BUILD_DIR)/entry.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/main.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/vmm.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/shell.o

iso: $(ISO_IMAGE)

$(ISO_IMAGE): $(KERNEL_ELF)
	@mkdir -p "$(ISO_DIR)/boot" "$(ISO_DIR)/boot/grub"
	cp "$(KERNEL_ELF)" "$(ISO_KERNEL)"
	grub-mkrescue -o "$(ISO_IMAGE)" "$(ISO_DIR)" >/dev/null
	@echo "Built: $(ISO_IMAGE)"

run: $(ISO_IMAGE)
	qemu-system-x86_64 -m 256M -cdrom "$(ISO_IMAGE)"

clean:
	rm -rf "$(BUILD_DIR)" "$(ISO_DIR)/boot/kernel.elf"

