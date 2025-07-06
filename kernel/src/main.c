#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "font.h"
#include "draw/draw.h"
#include "cpu/interrupts/idt.h"
#include "cpu/gdt/gdt.h"
#include "cpu/pic/pic.h"
#include "cpu/keyboard/keyboard.h"
#include "serial/serial.h"
#include "timing/timer.h"
#include "memory.h"
#include "pc_speaker/speaker.h"
#include "cpu/cpuid/cpuid.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_color = rgb_to_color(255, 255, 255);
    uint64_t total_memory = detect_total_memory();
    
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    init_serial();
    
    init_framebuffer();
    
    for (int y = 0; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            fb_ptr[y * fb_pitch + x] = background_color;
        }
    }

    gdt_init();
    gdt_load();
    serial_printf("GDT loaded\r\n");

    idt_init();
    install_exceptions(); // Install CPU exceptions
    init_timer_irq();     // Install timer IRQ handler
    init_keyboard_irq();  // Install keyboard IRQ handler
    idt_load();           // Load IDT into CPU
    
    init_pic();           // Initialize PIC
    init_timer();         // Initialize PIT
    init_keyboard();      // Initialize keyboard
    init_timer_interrupts(); // Enable timer IRQ
    print_vendor();

    __asm__ volatile ("sti"); // Enable interrupts
    write_serial("Interrupts enabled");

    draw_string_center_screen("Hello world!", text_color);

    beep(750,  8);
    beep(850,  10);
    beep(1050, 12);

    while (1) {
        __asm__ volatile ("hlt");  // Halt until interrupt
    }
}