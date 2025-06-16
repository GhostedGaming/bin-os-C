#include "idt.h"
#include "../pic/pic.h"
#include "../../serial/serial.h"

// IDT array and register - aligned for performance
__attribute__((aligned(0x10)))
static idt_entry_t idt[256];
static idtr_t idtr;

// Forward declarations of interrupt handlers from isr.asm
extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void), isr5(void);
extern void isr6(void), isr7(void), isr8(void), isr9(void), isr10(void), isr11(void);
extern void isr12(void), isr13(void), isr14(void), isr15(void), isr16(void), isr17(void);
extern void isr18(void), isr19(void), isr20(void), isr21(void), isr22(void), isr23(void);
extern void isr24(void), isr25(void), isr26(void), isr27(void), isr28(void), isr29(void);
extern void isr30(void), isr31(void);

// Hardware interrupt handlers
extern void irq0(void), irq1(void);

// Forward declaration of isr_install
void isr_install(void);

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

// Install all ISRs into the IDT
void isr_install(void) {
    // Install all CPU exception handlers (0-31)
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);   // Division by zero
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);   // Debug
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);   // NMI
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);   // Breakpoint
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);   // Overflow
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);   // Bound range exceeded
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);   // Invalid opcode
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);   // Device not available
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);   // Double fault
    idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E);   // Coprocessor segment overrun
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E); // Invalid TSS
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E); // Segment not present
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E); // Stack-segment fault
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E); // General protection fault
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E); // Page fault
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E); // Reserved
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E); // x87 floating-point exception
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E); // Alignment check
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E); // Machine check
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E); // SIMD floating-point exception
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E); // Virtualization exception
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E); // Control protection exception
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E); // Reserved
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E); // Reserved
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E); // Reserved
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E); // Reserved
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E); // Reserved
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E); // Reserved
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E); // Reserved
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E); // Reserved
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E); // Security exception
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E); // Reserved
    
    // Install hardware interrupt handlers (after PIC remap to 32-47)
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E);  // Timer (IRQ0)
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E);  // Keyboard (IRQ1)
    
    write_serial("IDT: All interrupt handlers installed");
}

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
    
    write_serial("IDT: Installing interrupt handlers...");
    isr_install(); // Install all ISR handlers
}

// Load IDT function
void idt_load(void) {
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    write_serial("IDT: IDT loaded into CPU");
}

// Basic interrupt handlers (called from assembly)
void isr_handler(uint64_t interrupt_number) {
    write_serial("CPU Exception occurred!");
    
    // You can add specific handling based on interrupt_number here
    switch(interrupt_number) {
        case 0:
            write_serial("Division by zero exception");
            break;
        case 6:
            write_serial("Invalid opcode exception");
            break;
        case 8:
            write_serial("Double fault exception");
            break;
        case 13:
            write_serial("General protection fault");
            break;
        case 14:
            write_serial("Page fault exception");
            break;
        default:
            write_serial("Unknown CPU exception");
            break;
    }
    
    // For now, just halt the system
    write_serial("System halted due to exception");
    __asm__ volatile ("cli; hlt");
}

void irq_handler(uint64_t irq_number) {
    // Handle hardware interrupt
    switch(irq_number) {
        case 32: // Timer interrupt
            // Don't spam serial with timer messages
            break;
        case 33: // Keyboard interrupt
            write_serial("Keyboard interrupt");
            break;
        default:
            write_serial("Unknown hardware interrupt");
            break;
    }
    
    // Send EOI to PIC
    if (irq_number >= 32 && irq_number <= 47) {
        PIC_sendEOI(irq_number - 32);
    }
}