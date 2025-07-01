#include <stdint.h>
#include "timer.h"
#include "../io.h"
#include "../serial/serial.h"
#include "../cpu/cpuid/cpuid.h"
#include "../cpu/interrupts/idt.h"

const uint32_t freq = 100;

uint64_t ticks;

void on_irq0() {
    ticks += 1;
    write_serial("Timer ticked");
}

void init_timer() {
    ticks = 0;

    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    write_serial("Timer handler installed\n");
}