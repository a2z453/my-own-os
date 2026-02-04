#include <stdint.h>
#include <stddef.h>

// VGA text mode buffer is at physical 0xB8000.
// In this GRUB/Multiboot2 setup we build our own page tables and include an
// identity mapping for the first 2 MiB, so 0xB8000 is directly accessible.
static inline volatile uint16_t *vga_buffer(void) {
    return (volatile uint16_t *)(uintptr_t)0xB8000ULL;
}

static void vga_clear(volatile uint16_t *vga) {
    const uint8_t attr = 0x0F; // white-on-black
    const uint16_t blank = (uint16_t)(' ' | ((uint16_t)attr << 8));
    for (size_t i = 0; i < 80 * 25; i++) {
        vga[i] = blank;
    }
}

static void vga_write_string(volatile uint16_t *vga, size_t row, size_t col, const char *s) {
    const uint8_t attr = 0x0F; // white-on-black
    size_t i = row * 80 + col;
    while (*s) {
        vga[i++] = (uint16_t)((uint8_t)(*s++) | ((uint16_t)attr << 8));
    }
}

// Called from `entry.asm` after we enter long mode.
void kmain(void) {
    volatile uint16_t *vga = vga_buffer();
    vga_clear(vga);
    vga_write_string(vga, 0, 0, "Hello, OS World!");

    // Halt forever.
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

