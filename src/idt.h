#pragma once

#include <stdint.h>

// IDT gate types.
// 0x8E = present | ring0 | interrupt gate (type 0xE)
#define IDT_TYPE_INT_GATE 0x8E

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint64_t base;
};

void idt_init(void);

// C handler called from assembly stubs.
struct __attribute__((packed)) isr_context {
    // Must match the exact push order in `src/interrupts.asm` where RSP is
    // passed to C.
    uint64_t rax, rbx, rcx, rdx, rbp, rdi, rsi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t vector;
    uint64_t error;
    uint64_t rip, cs, rflags;
};

void isr_handler(struct isr_context *ctx);

