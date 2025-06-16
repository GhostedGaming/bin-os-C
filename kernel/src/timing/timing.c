#include "timing.h"
#include "../serial/serial.h"

// Simple timing system using a basic counter
static volatile uint64_t system_ticks = 0;
static uint64_t ticks_per_second = 1000; // Assume 1000 ticks per second for simplicity

void init_timing(void) {
    system_ticks = 0;
    write_serial("Timing system initialized (simple counter-based)");
}

uint64_t get_uptime_ms(void) {
    // Simple implementation - just return ticks
    // In a real system, this would use the TSC or timer interrupts
    return system_ticks;
}

void sleep_ms(uint64_t milliseconds) {
    // Simple busy-wait implementation
    // In a real system, this would use proper timer interrupts
    uint64_t start = system_ticks;
    uint64_t target = start + milliseconds;
    
    // Busy wait - not efficient but works for basic functionality
    volatile uint64_t counter = 0;
    while (counter < milliseconds * 10000) {
        counter++;
    }
    
    system_ticks = target;
}

void format_uptime(char* buffer, int buffer_size) {
    if (buffer_size < 16) return;
    
    uint64_t uptime_ms = get_uptime_ms();
    uint64_t seconds = uptime_ms / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    // Simple formatting - just show seconds for now
    // In a real implementation, you'd want proper sprintf functionality
    if (hours > 0) {
        // Format: "Xh Ym Zs"
        int pos = 0;
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (hours % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 'h';
        if (pos < buffer_size - 1) buffer[pos++] = ' ';
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (minutes % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 'm';
        if (pos < buffer_size - 1) buffer[pos++] = ' ';
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (seconds % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 's';
        buffer[pos] = '\0';
    } else if (minutes > 0) {
        // Format: "Xm Ys"
        int pos = 0;
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (minutes % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 'm';
        if (pos < buffer_size - 1) buffer[pos++] = ' ';
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (seconds % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 's';
        buffer[pos] = '\0';
    } else {
        // Format: "Xs"
        int pos = 0;
        if (seconds >= 10) {
            if (pos < buffer_size - 1) buffer[pos++] = '0' + (seconds / 10);
        }
        if (pos < buffer_size - 1) buffer[pos++] = '0' + (seconds % 10);
        if (pos < buffer_size - 1) buffer[pos++] = 's';
        buffer[pos] = '\0';
    }
}

// Function to increment system ticks (would be called from timer interrupt)
void tick_increment(void) {
    system_ticks++;
}
