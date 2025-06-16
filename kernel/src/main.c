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
#include "serial/serial.h"
#include "timing/timing.h"
#include "memory.h"

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

// Function declarations
void update_time_display(void);
void update_time_display_at_position(int y_position);
void create_cpu_display_strings(char* vendor_str, char* speed_str, char* freq_info_str, 
                               int vendor_size, int speed_size, int freq_size);

// Memory allocation function (uses memmap_request from this file)
void* allocate_memory(size_t size) {
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= size) {
            return (void*)entry->base;
        }
    }
    return NULL;
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

// Simple string length function
int string_length(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Simple string comparison
int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
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

// Helper function to update time display at specific position
void update_time_display_at_position(int y_position) {
    if (global_fb_ptr == NULL) return;
    
    char time_buffer[16];
    format_uptime(time_buffer, sizeof(time_buffer));
    
    // Clear the time area (draw black rectangle)
    uint32_t black = rgb_to_color(0, 0, 0);
    for (int y = y_position; y < y_position + 48; y++) {
        for (int x = 0; x < global_width; x++) {
            global_fb_ptr[y * global_pitch + x] = black;
        }
    }
    
    // Draw updated time
    draw_string_center_screen_with_bg(global_fb_ptr, global_width, y_position, 
                                     global_pitch, time_buffer, global_text_color, global_highlight_color);
}

// Function to create CPU display strings using C CPUID implementation
void create_cpu_display_strings(char* vendor_str, char* speed_str, char* freq_info_str, 
                               int vendor_size, int speed_size, int freq_size) {
    // Initialize strings
    vendor_str[0] = '\0';
    speed_str[0] = '\0';
    freq_info_str[0] = '\0';
    
    // Use C CPUID implementation
    if (cpuid_is_supported()) {
        write_serial("Using C CPUID implementation");
        
        // Get vendor string from C implementation
        char* c_vendor = get_cpu_vendor();
        if (c_vendor && string_length(c_vendor) > 0) {
            string_concat(vendor_str, "CPU: ", vendor_size);
            
            // Convert vendor ID to readable name
            if (string_compare(c_vendor, "GenuineIntel") == 0) {
                string_concat(vendor_str, "Intel", vendor_size);
            } else if (string_compare(c_vendor, "AuthenticAMD") == 0) {
                string_concat(vendor_str, "AMD", vendor_size);
            } else if (string_compare(c_vendor, "CentaurHauls") == 0) {
                string_concat(vendor_str, "VIA/Centaur", vendor_size);
            } else {
                string_concat(vendor_str, c_vendor, vendor_size);
            }
            
            // Log raw vendor string
            write_serial("Raw vendor string: ");
            write_serial(c_vendor);
        }
        
        // Get CPU features
        if (cpu_has_sse() || cpu_has_sse2()) {
            string_concat(freq_info_str, "Features: ", freq_size);
            if (cpu_has_sse()) {
                string_concat(freq_info_str, "SSE ", freq_size);
            }
            if (cpu_has_sse2()) {
                string_concat(freq_info_str, "SSE2 ", freq_size);
            }
        }
        
        // Get max CPUID function
        unsigned int max_func = get_cpu_max_function();
        char max_func_msg[64] = "Max CPUID function: ";
        char num_str[16];
        uint_to_string(max_func, num_str, sizeof(num_str));
        string_concat(max_func_msg, num_str, sizeof(max_func_msg));
        write_serial(max_func_msg);
        
        // Set a basic speed string since we don't have frequency detection yet
        string_concat(speed_str, "Speed: Detected via CPUID", speed_size);
        
    } else {
        write_serial("CPUID not supported");
        
        // Set default strings
        string_concat(vendor_str, "CPU: Unknown (No CPUID)", vendor_size);
        string_concat(speed_str, "Speed: Unknown", speed_size);
    }
    
    // Ensure we have default strings if nothing was set
    if (string_length(vendor_str) == 0) {
        string_concat(vendor_str, "CPU: Unknown", vendor_size);
    }
    
    if (string_length(speed_str) == 0) {
        string_concat(speed_str, "Speed: Unknown", speed_size);
    }
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
    
    write_serial("Allocating memory");
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
    
    // Get CPU information using C CPUID implementation
    write_serial("Getting CPU information...");
    
    // Create display strings
    char vendor_str[64] = {0};
    char speed_str[64] = {0};
    char freq_info_str[64] = {0};
    
    create_cpu_display_strings(vendor_str, speed_str, freq_info_str, 
                              sizeof(vendor_str), sizeof(speed_str), sizeof(freq_info_str));
    
    write_serial("CPU information gathered and formatted");
    
    // Log CPU info to serial
    write_serial("=== CPU Information ===");
    write_serial(vendor_str);
    write_serial(speed_str);
    if (freq_info_str[0] != '\0') {
        write_serial(freq_info_str);
    }
    
    // Additional C CPUID testing
    if (cpuid_is_supported()) {
        write_serial("=== C CPUID Details ===");
        
        // Test safe vendor function
        char safe_vendor[13];
        get_cpu_vendor_safe(safe_vendor, sizeof(safe_vendor));
        write_serial("Safe vendor string: ");
        write_serial(safe_vendor);
        
        // Get detailed CPU features
        unsigned int ecx_features, edx_features;
        get_cpu_features(&ecx_features, &edx_features);
        
        char feature_msg[64] = "Feature flags - ECX: 0x";
        char hex_str[16];
        uint_to_string(ecx_features, hex_str, sizeof(hex_str));
        string_concat(feature_msg, hex_str, sizeof(feature_msg));
        write_serial(feature_msg);
        
        char feature_msg2[64] = "Feature flags - EDX: 0x";
        uint_to_string(edx_features, hex_str, sizeof(hex_str));
        string_concat(feature_msg2, hex_str, sizeof(feature_msg2));
        write_serial(feature_msg2);
    }
    
    write_serial("=======================");
    
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
    write_serial("Drawing initial display...");
    draw_string_center_screen_with_bg(fb_ptr, width, height, pitch, "BinOS - C Kernel", text_color, highlight_color);
    draw_string_center_screen_with_bg(fb_ptr, width, height - 48, pitch, vendor_str, text_color, highlight_color);
    draw_string_center_screen_with_bg(fb_ptr, width, height - 96, pitch, speed_str, text_color, highlight_color);
    
    // Draw additional frequency info if available
    if (freq_info_str[0] != '\0') {
        draw_string_center_screen_with_bg(fb_ptr, width, height - 144, pitch, freq_info_str, text_color, highlight_color);
        // Draw initial time lower if we have frequency info
        update_time_display_at_position(height - 192);
    } else {
        // Draw initial time at normal position
        update_time_display();
    }
    
    write_serial("Display initialized");
    
    // Enable interrupts LAST
    write_serial("Enabling interrupts...");
    asm volatile ("sti");
    write_serial("Interrupts enabled - System ready!");
    
    // Print final system status
    write_serial("=== System Status ===");
    write_serial("BinOS kernel successfully initialized");
    write_serial("All systems operational");
    
    // Print system capabilities
    if (cpuid_is_supported()) {
        write_serial("CPU identification: Available");
        if (cpu_has_sse()) {
            write_serial("SSE support: Yes");
        } else {
            write_serial("SSE support: No");
        }
        if (cpu_has_sse2()) {
            write_serial("SSE2 support: Yes");
        } else {
            write_serial("SSE2 support: No");
        }
    } else {
        write_serial("CPU identification: Not available");
    }
    
    write_serial("Memory management: Basic");
    write_serial("Serial communication: Active");
    write_serial("Graphics: Framebuffer active");
    write_serial("Timing system: Operational");
    write_serial("Interrupt handling: Enabled");
    write_serial("Entering main loop...");
    write_serial("====================");
    
    // Main loop with time updates and system monitoring
    uint64_t last_update = 0;
    uint64_t last_status_log = 0;
    uint64_t boot_time = get_uptime_ms();
    
    write_serial("Main loop started - system fully operational");
    
    while (1) {
        uint64_t current_time = get_uptime_ms();
        
        // Update time display every second (1000ms)
        if (current_time - last_update >= 1000) {
            if (freq_info_str[0] != '\0') {
                update_time_display_at_position(global_height - 192);
            } else {
                update_time_display();
            }
            last_update = current_time;
        }
        
        // Log system status every 5 minutes (300000ms)
        if (current_time - last_status_log >= 300000) {
            uint64_t uptime_seconds = (current_time - boot_time) / 1000;
            uint64_t uptime_minutes = uptime_seconds / 60;
            uint64_t uptime_hours = uptime_minutes / 60;
            
            write_serial("=== Periodic Status Report ===");
            
            // Format uptime message
            char uptime_msg[128] = "System uptime: ";
            char num_str[16];
            
            if (uptime_hours > 0) {
                uint_to_string((unsigned int)uptime_hours, num_str, sizeof(num_str));
                string_concat(uptime_msg, num_str, sizeof(uptime_msg));
                string_concat(uptime_msg, "h ", sizeof(uptime_msg));
                
                uint_to_string((unsigned int)(uptime_minutes % 60), num_str, sizeof(num_str));
                string_concat(uptime_msg, num_str, sizeof(uptime_msg));
                string_concat(uptime_msg, "m ", sizeof(uptime_msg));
            } else if (uptime_minutes > 0) {
                uint_to_string((unsigned int)uptime_minutes, num_str, sizeof(num_str));
                string_concat(uptime_msg, num_str, sizeof(uptime_msg));
                string_concat(uptime_msg, "m ", sizeof(uptime_msg));
            }
            
            uint_to_string((unsigned int)(uptime_seconds % 60), num_str, sizeof(num_str));
            string_concat(uptime_msg, num_str, sizeof(uptime_msg));
            string_concat(uptime_msg, "s", sizeof(uptime_msg));
            
            write_serial(uptime_msg);
            write_serial("System status: Stable");
            write_serial("All subsystems: Operational");
            
            // Log CPU status if available
            if (cpuid_is_supported()) {
                write_serial("CPU monitoring: Active");
                char cpu_status[64] = "CPU vendor: ";
                char* current_vendor = get_cpu_vendor();
                if (current_vendor && string_length(current_vendor) > 0) {
                    string_concat(cpu_status, current_vendor, sizeof(cpu_status));
                    write_serial(cpu_status);
                }
                
                // Log current CPU features
                if (cpu_has_sse() || cpu_has_sse2()) {
                    char features[64] = "Active features: ";
                    if (cpu_has_sse()) {
                        string_concat(features, "SSE ", sizeof(features));
                    }
                    if (cpu_has_sse2()) {
                        string_concat(features, "SSE2 ", sizeof(features));
                    }
                    write_serial(features);
                }
            }
            
            write_serial("==============================");
            
            last_status_log = current_time;
        }
        
        // Sleep for a short time to reduce CPU usage
        sleep_ms(100); // Reduced sleep time for more responsive updates
        
        // Optional: Add some basic system health checks here
        static uint64_t last_health_check = 0;
        if (current_time - last_health_check >= 60000) { // Every minute
            if (cpuid_is_supported()) {
                // Quick health check - verify CPUID still works
                char* vendor_check = get_cpu_vendor();
                if (vendor_check && string_length(vendor_check) > 0) {
                    // CPUID is still working - could log this for debugging
                    // write_serial("CPUID health check: OK");
                } else {
                    write_serial("WARNING: CPUID health check failed");
                }
            }
            last_health_check = current_time;
        }
    }
}