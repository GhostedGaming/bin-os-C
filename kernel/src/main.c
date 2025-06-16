#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "font.h"
#include "draw/draw.h"
#include "cpu/interrupts/idt.h"
#include "cpu/cpuid/print_vendor.h"
#include "cpu/gdt/gdt.h"
#include "cpu/pic/pic.h"
#include "serial/serial.h"
#include "timing/timing.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;
    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}

static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void* allocate_memory(size_t size) {
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= size) {
            return (void*)entry->base;
        }
    }
    return NULL;
}

// Simple integer to string conversion for displaying numbers
int uint_to_string(unsigned int value, char* buffer, int buffer_size) {
    if (buffer_size < 2) return 0; // Need at least space for '0' and null terminator
    
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }
    
    char temp[12]; // Enough for 32-bit unsigned int
    int temp_pos = 0;
    
    // Convert digits in reverse order
    while (value > 0 && temp_pos < 11) {
        temp[temp_pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Check if we have enough space in the output buffer
    if (temp_pos >= buffer_size) {
        return 0; // Not enough space
    }
    
    // Copy digits in correct order
    for (int i = 0; i < temp_pos; i++) {
        buffer[i] = temp[temp_pos - 1 - i];
    }
    buffer[temp_pos] = '\0';
    return temp_pos;
}

// Simple string concatenation
void string_concat(char* dest, const char* src, int dest_size) {
    int dest_len = 0;
    
    // Find end of destination string
    while (dest[dest_len] != '\0' && dest_len < dest_size - 1) {
        dest_len++;
    }
    
    // Append source string
    int src_pos = 0;
    while (src[src_pos] != '\0' && dest_len < dest_size - 1) {
        dest[dest_len++] = src[src_pos++];
    }
    dest[dest_len] = '\0';
}

// Global variables for display updates
static uint32_t *global_fb_ptr = NULL;
static int global_width = 0;
static int global_height = 0;
static int global_pitch = 0;
static uint32_t global_text_color = 0;
static uint32_t global_highlight_color = 0;

// Function to update the time display
void update_time_display(void) {
    if (global_fb_ptr == NULL) return;
    
    char time_buffer[16];
    format_uptime(time_buffer, sizeof(time_buffer));
    
    // Clear the time area (draw black rectangle)
    uint32_t black = rgb_to_color(0, 0, 0);
    for (int y = global_height - 144; y < global_height - 96; y++) {
        for (int x = 0; x < global_width; x++) {
            global_fb_ptr[y * global_pitch + x] = black;
        }
    }
    
    // Draw updated time
    draw_string_center_screen_with_bg(global_fb_ptr, global_width, global_height - 144, 
                                     global_pitch, time_buffer, global_text_color, global_highlight_color);
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_color = rgb_to_color(255, 255, 255);  // White text
    uint32_t highlight_color = rgb_to_color(185, 185, 189); // Grey highlight
    
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    
    init_serial();
    write_serial("Serial initialized");
    
    write_serial("Allocating memory.");
    allocate_memory(1024 * 1024);
    write_serial("Memory allocated");
    
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    uint32_t *fb_ptr = framebuffer->address;
    int width = framebuffer->width;
    int height = framebuffer->height;
    int pitch = framebuffer->pitch / 4;
    
    // Store globals for time updates
    global_fb_ptr = fb_ptr;
    global_width = width;
    global_height = height;
    global_pitch = pitch;
    global_text_color = text_color;
    global_highlight_color = highlight_color;
    
    write_serial("Limine framebuffer initialized");
    
    // Fill background
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb_ptr[y * pitch + x] = background_color;
        }
    }
    
    write_serial("Background filled");
    
    // Initialize GDT
    gdt_init();
    write_serial("GDT initialized");
    gdt_load();
    write_serial("GDT loaded");
    
    // Initialize timing system
    write_serial("Initializing timing system...");
    init_timing();
    write_serial("Timing system initialized");
    
    // Get CPU information
    write_serial("Getting CPU information...");
    char* vendor = process_cpu_vendor();
    unsigned int cpu_speed = get_cpu_base_frequency();
    
    // Create speed string
    char speed_str[64] = "CPU Speed: ";
    char speed_num[16] = {0};
    if (cpu_speed > 0) {
        uint_to_string(cpu_speed, speed_num, sizeof(speed_num));
        string_concat(speed_str, speed_num, sizeof(speed_str));
        string_concat(speed_str, " MHz", sizeof(speed_str));
    } else {
        string_concat(speed_str, "Unknown", sizeof(speed_str));
    }
    
    write_serial("CPU information gathered");
    
    // Initialize interrupt system (IMPORTANT: Do this BEFORE enabling interrupts)
    write_serial("Initializing interrupt system...");
    
    // Initialize IDT
    idt_init();
    write_serial("IDT initialized");
    
    // Remap PIC to avoid conflicts with CPU exceptions
    PIC_remap(32, 40);
    write_serial("PIC remapped");
    
    // Load IDT
    idt_load();
    write_serial("IDT loaded");
    
    // Draw initial display
    draw_string_center_screen_with_bg(fb_ptr, width, height, pitch, "BinOS - C Kernel", text_color, highlight_color);
    draw_string_center_screen_with_bg(fb_ptr, width, height - 48, pitch, vendor, text_color, highlight_color);
    draw_string_center_screen_with_bg(fb_ptr, width, height - 96, pitch, speed_str, text_color, highlight_color);
    
    // Draw initial time
    update_time_display();
    
    write_serial("Display initialized");
    
    // Enable interrupts LAST
    write_serial("Enabling interrupts...");
    asm volatile ("sti");
    write_serial("Interrupts enabled - System ready!");
    
    // Main loop with time updates
    uint64_t last_update = 0;
    while (1) {
        uint64_t current_time = get_uptime_ms();
        
        // Update time display every second (1000ms)
        if (current_time - last_update >= 1000) {
            update_time_display();
            last_update = current_time;
        }
        
        // Sleep for a short time to reduce CPU usage
        sleep_ms(10);
    }
}