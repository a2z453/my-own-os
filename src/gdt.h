#pragma once

#include <stdint.h>

// Minimal x86_64 GDT setup for long mode.
// In long mode, segmentation is mostly disabled, but CS still needs a valid
// 64-bit code descriptor and the data segments should reference a valid
// data descriptor.

void gdt_init(void);

// Segment selectors (must match the GDT we build).
enum {
    GDT_KERNEL_CODE_SEL = 0x08,
    GDT_KERNEL_DATA_SEL = 0x10,
};

