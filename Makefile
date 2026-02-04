ARCH := x86_64
CROSS_COMPILE ?= x86_64-elf-
CC ?= $(CROSS_COMPILE)gcc
LD ?= $(CROSS_COMPILE)ld
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NASM := nasm
XORRISO := xorriso

KERNEL := build/kernel.elf
ISO := build/my-own-os.iso
LIMINE_DIR := limine

CFLAGS := -Wall -Wextra -Werror -std=c11 -ffreestanding -fno-stack-protector \
	-fno-pic -mno-red-zone -mcmodel=kernel -Iinclude
LDFLAGS := -T linker.ld
NASMFLAGS := -f elf64

OBJS := \
	build/start.o \
	build/kernel.o \
	build/serial.o \
	build/panic.o

.PHONY: all iso clean limine

all: $(KERNEL)

build:
	mkdir -p build

build/start.o: arch/$(ARCH)/boot/start.asm | build
	$(NASM) $(NASMFLAGS) $< -o $@

build/kernel.o: kernel/src/kernel.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/serial.o: kernel/src/serial.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/panic.o: kernel/src/panic.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): toolchain-check $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

limine: toolchain-check
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

limine:
	@if [ ! -d $(LIMINE_DIR) ]; then \
		git clone https://github.com/limine-bootloader/limine.git $(LIMINE_DIR); \
	fi
	$(MAKE) -C $(LIMINE_DIR)

toolchain-check:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "error: $(CC) not found. Install the x86_64-elf toolchain or set CC to a valid cross-compiler."; \
		exit 1; \
	}
	@command -v $(LD) >/dev/null 2>&1 || { \
		echo "error: $(LD) not found. Install the x86_64-elf binutils or set LD to a valid linker."; \
		exit 1; \
	}

iso: $(KERNEL) limine
	mkdir -p iso/boot
	cp $(KERNEL) iso/boot/kernel.elf
	cp limine.cfg iso/boot/limine.cfg
	cp $(LIMINE_DIR)/limine-bios.sys iso/boot/
	cp $(LIMINE_DIR)/limine-bios-cd.bin iso/boot/
	cp $(LIMINE_DIR)/limine-uefi-cd.bin iso/boot/
	$(XORRISO) -as mkisofs -b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso -o $(ISO)
	$(LIMINE_DIR)/limine-deploy $(ISO)

clean:
	rm -rf build iso
