#include <stdint.h>
#include <stddef.h>

#include "gdt.h"
#include "idt.h"
#include "multiboot2.h"
#include "pic.h"
#include "pmm.h"
#include "vmm.h"
#include "keyboard.h"
#include "shell.h"

// Very small VGA text-mode writer (white on black).
// Note: After VMM init, we'll use vmm_framebuffer instead
static volatile uint16_t *VGA = (volatile uint16_t *)(uintptr_t)0xB8000;
static const uint8_t VGA_ATTR = 0x0F;

static void vga_clear(void) {
    const uint16_t blank = (uint16_t)(' ' | ((uint16_t)VGA_ATTR << 8));
    for (size_t i = 0; i < 80 * 25; i++) {
        VGA[i] = blank;
    }
}

static void vga_write_at(size_t row, size_t col, const char *s) {
    size_t i = row * 80 + col;
    while (*s) {
        VGA[i++] = (uint16_t)((uint8_t)(*s++) | ((uint16_t)VGA_ATTR << 8));
    }
}

// Optional serial output (useful when running QEMU headless with -nographic).
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_init(void) {
    // COM1 = 0x3F8
    outb(0x3F8 + 1, 0x00); // Disable interrupts
    outb(0x3F8 + 3, 0x80); // Enable DLAB
    outb(0x3F8 + 0, 0x03); // Divisor low  (38400 baud)
    outb(0x3F8 + 1, 0x00); // Divisor high
    outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
    outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static void serial_write_char(char c) {
    // Wait for transmitter holding register empty.
    while ((inb(0x3F8 + 5) & 0x20) == 0) {}
    outb(0x3F8, (uint8_t)c);
}

static void serial_write(const char *s) {
    while (*s) serial_write_char(*s++);
}

static void print_hex64(uint64_t val) {
    char buf[17];
    const char *hex = "0123456789ABCDEF";
    for (int i = 15; i >= 0; --i) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[16] = 0;
    serial_write("0x");
    serial_write(buf);
}

void isr_handler(struct isr_context *ctx) {
    if (ctx->vector == 33) {
        // Keyboard IRQ
        extern void keyboard_irq_handler(void);
        keyboard_irq_handler();
        return; // Don't halt, continue execution
    }
    
    if (ctx->vector == 0) {
        vga_clear();
        vga_write_at(0, 0, "EXCEPTION CAUGHT");
        vga_write_at(1, 0, "Divide by Zero");
        serial_write("EXCEPTION CAUGHT: Divide by Zero\r\n");
    } else if (ctx->vector == 13) {
        vga_clear();
        vga_write_at(0, 0, "#GP FAULT");
        vga_write_at(1, 0, "General Protection");
        serial_write("#GP: error=");
        print_hex64(ctx->error);
        serial_write(" RIP=");
        print_hex64(ctx->rip);
        serial_write("\r\n");
    } else if (ctx->vector == 14) {
        vga_clear();
        vga_write_at(0, 0, "#PF FAULT");
        vga_write_at(1, 0, "Page Fault");
        serial_write("#PF: error=");
        print_hex64(ctx->error);
        serial_write(" RIP=");
        print_hex64(ctx->rip);
        serial_write("\r\n");
    }

    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

static void print_hex(uint64_t val) {
    char buf[17];
    const char *hex = "0123456789ABCDEF";
    for (int i = 15; i >= 0; --i) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[16] = 0;
    vga_write_at(2, 0, "Free: 0x");
    vga_write_at(2, 8, buf);
    vga_write_at(2, 24, " bytes");
}

// Called from kernel/entry.asm after long mode is enabled.
void kmain(uint64_t mb_info_addr, uint32_t mb_magic) {
    serial_init();
    vga_clear();
    vga_write_at(0, 0, "Hello, OS World!");

    gdt_init();
    idt_init();
    pic_init(0x20, 0x28);  // Remap PIC to IRQ 0x20-0x2F

    if (mb_magic == MULTIBOOT2_MAGIC) {
        pmm_init(mb_info_addr);
        uint64_t free = pmm_free_bytes();
        print_hex(free);
        
        // Initialize VMM (paging)
        vmm_init();
        
        // Switch to virtual framebuffer
        VGA = vmm_framebuffer;
        
        // Initialize keyboard and shell
        keyboard_init();
        shell_init();
        
        // Unmask keyboard IRQ only
        uint8_t mask = inb(0x21);
        mask &= ~(1 << 1); // Enable keyboard IRQ
        outb(0x21, mask);
    } else {
        vga_write_at(1, 0, "Bad Multiboot2 magic");
    }

    // Enable interrupts
    __asm__ __volatile__("sti");

    // Main loop: process shell input
    for (;;) {
        shell_run();
        __asm__ __volatile__("hlt");
    }
}

