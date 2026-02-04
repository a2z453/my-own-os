#include "kernel/serial.h"

#include "arch/x86_64/io.h"

#define COM1_PORT 0x3F8

static int serial_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

void serial_write_char(char c) {
    while (!serial_transmit_empty()) {
        io_wait();
    }

    outb(COM1_PORT, (uint8_t)c);
}

void serial_write(const char *str) {
    if (!str) {
        return;
    }

    while (*str) {
        if (*str == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*str++);
    }
}
