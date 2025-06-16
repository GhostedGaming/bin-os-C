#include "io.h"
#include "serial.h"

#define PORT 0x3F8 // COM1

int init_serial(void) {
    outb(PORT + 1, 0x00); // Disable all interrupts
    outb(PORT + 3, 0x80); // Enable DLAB
    outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte)
    outb(PORT + 1, 0x00); // (hi byte)
    outb(PORT + 3, 0x03); // 8N1
    outb(PORT + 2, 0xC7); // Enable FIFO, clear them
    outb(PORT + 4, 0x0B); // IRQs enabled
    outb(PORT + 4, 0x1E); // Loopback test
    outb(PORT + 0, 0xAE); // Send test byte

    if (inb(PORT + 0) != 0xAE) {
        return 1; // Failed
    }

    outb(PORT + 4, 0x0F); // Normal mode
    return 0;
}

static int is_transmit_empty(void) {
    return inb(PORT + 5) & 0x20;
}

static void write_serial_char(char a) {
    while (is_transmit_empty() == 0);
    outb(PORT, a);
}

void write_serial(const char *str) {
    while (*str) {
        write_serial_char(*str++);
    }

    write_serial_char('\n');
    write_serial_char('\r');
}