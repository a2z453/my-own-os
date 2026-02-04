#include "keyboard.h"
#include "pic.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

// PS/2 scancode set 1 to ASCII (simplified, only common keys)
static char scancode_to_ascii(uint8_t scancode, int shift) {
    if (scancode >= 0x02 && scancode <= 0x0B) {
        const char *normal = "1234567890";
        const char *shifted = "!@#$%^&*()";
        return shift ? shifted[scancode - 0x02] : normal[scancode - 0x02];
    }
    if (scancode >= 0x10 && scancode <= 0x19) {
        const char *normal = "qwertyuiop";
        const char *shifted = "QWERTYUIOP";
        return shift ? shifted[scancode - 0x10] : normal[scancode - 0x10];
    }
    if (scancode >= 0x1E && scancode <= 0x26) {
        const char *normal = "asdfghjkl";
        const char *shifted = "ASDFGHJKL";
        return shift ? shifted[scancode - 0x1E] : normal[scancode - 0x1E];
    }
    if (scancode >= 0x2C && scancode <= 0x32) {
        const char *normal = "zxcvbnm";
        const char *shifted = "ZXCVBNM";
        return shift ? shifted[scancode - 0x2C] : normal[scancode - 0x2C];
    }
    if (scancode == 0x39) return ' ';
    if (scancode == 0x0E) return '\b'; // Backspace
    if (scancode == 0x1C) return '\n'; // Enter
    return 0;
}

static volatile int keyboard_has_char = 0;
static volatile char keyboard_char = 0;
static volatile int keyboard_shift = 0;

void keyboard_irq_handler(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        
        // Handle key release (0x80+)
        if (scancode & 0x80) {
            uint8_t key = scancode & 0x7F;
            if (key == 0x2A || key == 0x36) {
                keyboard_shift = 0;
            }
            return;
        }
        
        // Handle key press
        if (scancode == 0x2A || scancode == 0x36) {
            keyboard_shift = 1;
            return;
        }
        
        char c = scancode_to_ascii(scancode, keyboard_shift);
        if (c) {
            keyboard_char = c;
            keyboard_has_char = 1;
        }
    }
    
    // Send EOI to PIC
    outb(0x20, 0x20); // PIC1 EOI
}

void keyboard_init(void) {
    // Enable keyboard interrupt (IRQ 1)
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1); // Clear bit 1 (keyboard IRQ)
    outb(0x21, mask);
}

char keyboard_get_char(void) {
    if (keyboard_has_char) {
        keyboard_has_char = 0;
        return keyboard_char;
    }
    return 0;
}

int keyboard_has_data(void) {
    return keyboard_has_char;
}
