#include "shell.h"
#include "keyboard.h"
#include "vmm.h"
#include <stdint.h>
#include <stddef.h>

// VGA buffer (uses virtual framebuffer after VMM init)
extern volatile uint16_t *vmm_framebuffer;
static volatile uint16_t *vga_buffer = (volatile uint16_t *)(uintptr_t)0xB8000;
static const uint8_t VGA_ATTR = 0x0F;

static size_t cursor_row = 3;
static size_t cursor_col = 0;
static char input_buffer[256];
static size_t input_pos = 0;

static void vga_put_char(size_t row, size_t col, char c) {
    if (row < 25 && col < 80) {
        volatile uint16_t *buf = vmm_framebuffer ? vmm_framebuffer : vga_buffer;
        buf[row * 80 + col] = (uint16_t)((uint8_t)c | ((uint16_t)VGA_ATTR << 8));
    }
}

static void vga_scroll(void) {
    volatile uint16_t *buf = vmm_framebuffer ? vmm_framebuffer : vga_buffer;
    for (size_t row = 0; row < 24; row++) {
        for (size_t col = 0; col < 80; col++) {
            buf[row * 80 + col] = buf[(row + 1) * 80 + col];
        }
    }
    for (size_t col = 0; col < 80; col++) {
        buf[24 * 80 + col] = (uint16_t)(' ' | ((uint16_t)VGA_ATTR << 8));
    }
}

static void shell_print(const char *s) {
    while (*s) {
        if (*s == '\n') {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= 25) {
                vga_scroll();
                cursor_row = 24;
            }
        } else {
            vga_put_char(cursor_row, cursor_col, *s);
            cursor_col++;
            if (cursor_col >= 80) {
                cursor_col = 0;
                cursor_row++;
                if (cursor_row >= 25) {
                    vga_scroll();
                    cursor_row = 24;
                }
            }
        }
        s++;
    }
}

static void shell_print_prompt(void) {
    shell_print("> ");
    cursor_col = 2;
}

static void shell_execute_command(const char *cmd) {
    if (cmd[0] == '\0') {
        return;
    }
    
    shell_print("\n");
    
    // Simple command parsing
    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0') {
        shell_print("Commands:\n");
        shell_print("  help    - Show this help\n");
        shell_print("  clear   - Clear screen\n");
        shell_print("  testfb  - Test framebuffer write\n");
    } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0') {
        // Clear screen
        volatile uint16_t *buf = vmm_framebuffer ? vmm_framebuffer : vga_buffer;
        for (size_t i = 0; i < 80 * 25; i++) {
            buf[i] = (uint16_t)(' ' | ((uint16_t)VGA_ATTR << 8));
        }
        cursor_row = 0;
        cursor_col = 0;
        shell_print_prompt();
    } else if (cmd[0] == 't' && cmd[1] == 'e' && cmd[2] == 's' && cmd[3] == 't' && cmd[4] == 'f' && cmd[5] == 'b' && cmd[6] == '\0') {
        shell_print("Testing framebuffer write...\n");
        // This will test the virtual framebuffer mapping
        if (vmm_framebuffer) {
            vmm_framebuffer[0] = (uint16_t)('T' | ((uint16_t)0x0F << 8));
            vmm_framebuffer[1] = (uint16_t)('E' | ((uint16_t)0x0F << 8));
            vmm_framebuffer[2] = (uint16_t)('S' | ((uint16_t)0x0F << 8));
            vmm_framebuffer[3] = (uint16_t)('T' | ((uint16_t)0x0F << 8));
            shell_print("Wrote TEST to virtual framebuffer (top-left)\n");
        } else {
            shell_print("Framebuffer not mapped!\n");
        }
        shell_print_prompt();
    } else {
        shell_print("Unknown command: ");
        shell_print(cmd);
        shell_print("\nType 'help' for commands\n");
        shell_print_prompt();
    }
}

void shell_init(void) {
    shell_print("MyHobbyOS Shell v1.0\n");
    shell_print_prompt();
}

void shell_process_input(char c) {
    if (c == '\b') {
        // Backspace
        if (input_pos > 0) {
            input_pos--;
            input_buffer[input_pos] = '\0';
            if (cursor_col > 2) {
                cursor_col--;
                vga_put_char(cursor_row, cursor_col, ' ');
            }
        }
    } else if (c == '\n') {
        // Enter
        input_buffer[input_pos] = '\0';
        shell_execute_command(input_buffer);
        input_pos = 0;
    } else if (c >= 32 && c < 127 && input_pos < 255) {
        // Printable character
        input_buffer[input_pos++] = c;
        vga_put_char(cursor_row, cursor_col, c);
        cursor_col++;
        if (cursor_col >= 80) {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= 25) {
                vga_scroll();
                cursor_row = 24;
            }
        }
    }
}

void shell_run(void) {
    char c = keyboard_get_char();
    if (c) {
        shell_process_input(c);
    }
}
