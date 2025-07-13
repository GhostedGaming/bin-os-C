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
#include "shell/shell.h"
#include "timing/rtc/rtc.h"
#include "utility.h"

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

void test_memory_allocator(void) {
    serial_printf("Testing memory allocator...\r\n");
    
    void* ptr1 = malloc(100);
    if (ptr1) {
        serial_printf("Allocated 100 bytes at %p\r\n", ptr1);
    } else {
        serial_printf("Failed to allocate 100 bytes\r\n");
    }
    
    void* ptr2 = malloc(200);
    if (ptr2) {
        serial_printf("Allocated 200 bytes at %p\r\n", ptr2);
    } else {
        serial_printf("Failed to allocate 200 bytes\r\n");
    }
    
    void* ptr3 = calloc(10, sizeof(int));
    if (ptr3) {
        serial_printf("Allocated 40 bytes (calloc) at %p\r\n", ptr3);
    } else {
        serial_printf("Failed to allocate 40 bytes with calloc\r\n");
    }
    
    size_t total, used, free_size;
    get_heap_stats(&total, &used, &free_size);
    serial_printf("Heap stats: Total=%zu, Used=%zu, Free=%zu\r\n", total, used, free_size);
    
    free(ptr1);
    serial_printf("Freed ptr1\r\n");
    
    get_heap_stats(&total, &used, &free_size);
    serial_printf("After free: Total=%zu, Used=%zu, Free=%zu\r\n", total, used, free_size);
    
    void* ptr4 = malloc(150);
    if (ptr4) {
        serial_printf("Allocated 150 bytes at %p (should reuse freed space)\r\n", ptr4);
    } else {
        serial_printf("Failed to allocate 150 bytes\r\n");
    }
    
    void* ptr5 = realloc(ptr2, 400);
    if (ptr5) {
        serial_printf("Reallocated ptr2 to 400 bytes at %p\r\n", ptr5);
    } else {
        serial_printf("Failed to reallocate ptr2\r\n");
    }
    
    free(ptr3);
    free(ptr4);
    free(ptr5);
    
    get_heap_stats(&total, &used, &free_size);
    serial_printf("Final heap stats: Total=%zu, Used=%zu, Free=%zu\r\n", total, used, free_size);
    
    serial_printf("Memory allocator test completed\r\n");
}

int i = 0;

void update_screen() {
    i++;
    clear_screen(rgb_to_color(0, 0, 0));
    draw_string_center_screen(to_string(i), rgb_to_color(255, 255, 255));
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    
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
    
    serial_printf("Initializing memory allocator...\r\n");
    init_heap();
    serial_printf("Memory allocator initialized\r\n");
    
    gdt_init();
    gdt_load();
    serial_printf("GDT loaded\r\n");
    
    idt_init();
    install_exceptions();
    init_timer_irq();
    init_keyboard_irq();
    idt_load();
    init_pic();
    init_timer();
    init_keyboard();
    init_timer_interrupts();
    
    print_vendor();
    
    __asm__ volatile ("sti");
    write_serial("Interrupts enabled");
    
    test_memory_allocator();

    shell_init();

    display_current_time();

    while (1) {
        __asm__ volatile ("hlt");
        update_screen();
        timer_wait_seconds(1);
    }
}