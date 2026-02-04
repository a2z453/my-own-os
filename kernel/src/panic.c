#include "kernel/panic.h"

#include "kernel/serial.h"

void kernel_panic(const char *message) {
    serial_write("\n*** KERNEL PANIC ***\n");
    if (message) {
        serial_write(message);
        serial_write("\n");
    }

    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
