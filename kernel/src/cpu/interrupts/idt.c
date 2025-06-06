#include "idt.h"

// IDT array and register - aligned for performance
__attribute__((aligned(0x10)))
static idt_entry_t idt[256];
static idtr_t idtr;

// IDT initialization function
void idt_init(void) {
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt[i].isr_low = 0;
        idt[i].kernel_cs = 0;
        idt[i].ist = 0;
        idt[i].attributes = 0;
        idt[i].isr_mid = 0;
        idt[i].isr_high = 0;
        idt[i].reserved = 0;
    }
    
    // Set up IDTR
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;
}

// Set IDT gate function
void idt_set_gate(uint8_t vector, uint64_t isr, uint16_t selector, uint8_t flags) {
    idt[vector].isr_low = (uint16_t)(isr & 0xFFFF);
    idt[vector].kernel_cs = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid = (uint16_t)((isr >> 16) & 0xFFFF);
    idt[vector].isr_high = (uint32_t)((isr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}

// Load IDT function
void idt_load(void) {
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}