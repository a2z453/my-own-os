/* linker.ld - Multiboot2 kernel link script (loaded by GRUB).
 *
 * We link the kernel at 1 MiB and identity-map the first 2 MiB in paging,
 * which is enough for VGA (0xB8000) and our early code/data.
 */

ENTRY(_start)

SECTIONS {
    . = 1M;

    .text : ALIGN(0x1000) {
        *(.multiboot2)
        *(.text .text.*)
    }

    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(0x1000) {
        *(.data .data.*)
    }

    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss .bss.*)
    }

    _kernel_end = .;
    PROVIDE(kernel_end = .);
}

