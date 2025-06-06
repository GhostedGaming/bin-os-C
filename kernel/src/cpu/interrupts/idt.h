#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT Entry structure for x86-64
typedef struct {
    uint16_t isr_low;      // The lower 16 bits of the ISR's address
    uint16_t kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;           // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t attributes;    // Type and attributes; see the IDT page
    uint16_t isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;     // The higher 32 bits of the ISR's address
    uint32_t reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

// IDT Register structure
typedef struct {
    uint16_t limit;        // Size of IDT in bytes - 1
    uint64_t base;         // Base address of IDT
} __attribute__((packed)) idtr_t;

// IDT Attribute flags
#define IDT_INTERRUPT_GATE  0x8E
#define IDT_TRAP_GATE       0x8F
#define IDT_CALL_GATE       0x8C
#define IDT_PRESENT         0x80
#define IDT_DPL_0           0x00
#define IDT_DPL_1           0x20
#define IDT_DPL_2           0x40
#define IDT_DPL_3           0x60

// Function declarations
void idt_init(void);
void idt_set_gate(uint8_t vector, uint64_t isr, uint16_t selector, uint8_t flags);
void idt_load(void);

#endif // IDT_H