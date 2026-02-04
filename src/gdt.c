#include "gdt.h"

// GDT entry format (8 bytes).
struct __attribute__((packed)) gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
};

struct __attribute__((packed)) gdt_ptr {
    uint16_t limit;
    uint64_t base;
};

static struct gdt_entry gdt[3];
static struct gdt_ptr gdtr;

static struct gdt_entry gdt_make_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    struct gdt_entry e;
    e.limit_low   = (uint16_t)(limit & 0xFFFF);
    e.base_low    = (uint16_t)(base & 0xFFFF);
    e.base_mid    = (uint8_t)((base >> 16) & 0xFF);
    e.access      = access;
    e.granularity = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    e.base_high   = (uint8_t)((base >> 24) & 0xFF);
    return e;
}

void gdt_init(void) {
    // Null descriptor.
    gdt[0] = gdt_make_entry(0, 0, 0, 0);

    // Kernel code: present, ring0, executable, readable.
    // For 64-bit code segment: set L=1 (0x20 in granularity high nibble),
    // and G=1 for conventional (though limit ignored in long mode).
    gdt[1] = gdt_make_entry(0, 0xFFFFF, 0x9A, 0xA0); // 0xA0 => G=1, L=1

    // Kernel data: present, ring0, writable.
    // In long mode, the "default operand size" (DB) bit should be 0 for data
    // segments used as SS/DS/ES. We'll keep G=1 for a conventional limit field.
    gdt[2] = gdt_make_entry(0, 0xFFFFF, 0x92, 0x80); // 0x80 => G=1

    gdtr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtr.base  = (uint64_t)(uintptr_t)&gdt[0];

    // Load the new GDT.
    __asm__ __volatile__("lgdt %0" : : "m"(gdtr) : "memory");

    // Reload data segment registers. CS reload would require a far jump/iret;
    // in long mode we can usually keep the existing CS as long as it's valid.
    uint16_t data_sel = (uint16_t)GDT_KERNEL_DATA_SEL;
    __asm__ __volatile__(
        "movw %0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        :
        : "r"(data_sel)
        : "ax", "memory"
    );
}

