#include "../../serial/serial.h"
#include "../../io.h"

void keyboard_handler(struct interrupt_registers *regs) {
    uint8_t scancode = inb(0x60);
    uint8_t pressed = !(scancode & 0x80);
    scancode &= 0x7F;
    
    serial_printf("Key pressed - Scan code: %d\r\n", scancode);
}