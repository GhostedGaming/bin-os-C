#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "font.h"
#include "draw/draw.h"
#include "cpu/interrupts/idt.h"
#include "cpu/cpuid/cpuid.h"
#include "cpu/gdt/gdt.h"
#include "cpu/pic/pic.h"
#include "cpu/keyboard/keyboard.h"
#include "serial/serial.h"
#include "timing/timing.h"
#include "memory.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void* allocate_memory(size_t size) {
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= size) {
            return (void*)entry->base;
        }
    }
    return NULL;
}

void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_background = rgb_to_color(5,5,5);
    uint32_t text_color = rgb_to_color(255, 255, 255);
    
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    init_serial();
    
    // Initialize framebuffer first
    init_framebuffer();
    
    // Fill background using the global framebuffer variables
    for (int y = 0; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            fb_ptr[y * fb_pitch + x] = background_color;
        }
    }

    gdt_init();
    gdt_load();
    serial_printf("GDT loaded\r\n");

    idt_init();
    idt_load();
    write_serial("IDT loaded");

    PIC_remap(0x20, 0x28);

    outb(0x21, inb(0x21) & ~0x02);  // Clear bit 1 for IRQ1 on master PIC
    
    __asm__ volatile ("sti");  // Enable interrupts
    write_serial("Interrupts enabled");

    init_timer_irq();

    init_timing();

    init_keyboard();

    draw_string_center_screen_with_bg("New binbows", text_color, text_background);
    
    while (1) {
        __asm__ volatile ("hlt");  // Halt until interrupt
    }
}