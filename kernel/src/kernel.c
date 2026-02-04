#include "kernel/panic.h"
#include "kernel/serial.h"

void kmain(void) {
    serial_init();
    serial_write("[kernel] COM1 online.\n");

    kernel_panic("Reached kmain end without scheduler.");
}
