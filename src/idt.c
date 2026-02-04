#include "idt.h"
#include "gdt.h"

static struct idt_entry idt[256] __attribute__((aligned(16)));
static struct idt_ptr idtr;

extern void isr0(void);  // defined in interrupts.asm
extern void isr13(void); // defined in interrupts.asm
extern void isr14(void); // defined in interrupts.asm
extern void isr33(void); // defined in interrupts.asm (IRQ 1 - Keyboard)

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_putc(char c) {
    // COM1 = 0x3F8 (assumes already initialized in kmain)
    while ((inb(0x3F8 + 5) & 0x20) == 0) {}
    outb(0x3F8, (uint8_t)c);
}

static void idt_set_gate(int vec, void (*isr)(void), uint8_t type_attr) {
    uint64_t addr = (uint64_t)(uintptr_t)isr;
    idt[vec].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vec].selector    = GDT_KERNEL_CODE_SEL;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vec].zero        = 0;
}

void idt_init(void) {
    serial_putc('I'); serial_putc('0'); serial_putc('\r'); serial_putc('\n');
    // Zero all entries (byte loop to avoid any alignment-sensitive stores).
    volatile uint8_t *p = (volatile uint8_t *)&idt[0];
    for (uint64_t i = 0; i < (uint64_t)sizeof(idt); i++) {
        p[i] = 0;
    }

    // Exception handlers
    idt_set_gate(0,  isr0,  IDT_TYPE_INT_GATE);  // Divide-by-zero
    idt_set_gate(13, isr13, IDT_TYPE_INT_GATE);   // General Protection Fault
    idt_set_gate(14, isr14, IDT_TYPE_INT_GATE);   // Page Fault
    idt_set_gate(33, isr33, IDT_TYPE_INT_GATE);   // IRQ 1 (Keyboard)

    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)(uintptr_t)&idt[0];

    serial_putc('I'); serial_putc('1'); serial_putc('\r'); serial_putc('\n');
    __asm__ __volatile__("lidt %0" : : "m"(idtr) : "memory");
    serial_putc('I'); serial_putc('2'); serial_putc('\r'); serial_putc('\n');
    // NOTE: We intentionally do NOT `sti` yet.
    // Hardware IRQs (timer/keyboard/etc) will start firing immediately and,
    // until we install IRQ handlers + PIC/APIC setup, they'd cause triple faults.
}

