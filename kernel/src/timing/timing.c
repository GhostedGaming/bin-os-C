#include "timing.h"
#include "../serial/serial.h"
#include "../cpu/cpuid/cpuid.h"

static volatile uint64_t system_ticks = 0;
static uint64_t cpu_cycles_per_ms = 0;
static uint64_t start_tsc = 0;

static inline uint64_t read_tsc(void) {
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t)high << 32) | low;
}

void init_timing(void) {
    // uint32_t cpu_freq_mhz = get_cpu_frequency_mhz();
    
    if (200000 > 0) {
        cpu_cycles_per_ms = (uint64_t)200000 * 1000;
        start_tsc = read_tsc();
        write_serial("Timing: Using CPU frequency");
    } else {
        write_serial("Timing: Using fallback");
    }
}

uint64_t get_uptime_ms(void) {
    if (cpu_cycles_per_ms > 0) {
        uint64_t current_tsc = read_tsc();
        return (current_tsc - start_tsc) / cpu_cycles_per_ms;
    }
    return system_ticks;
}

void sleep_ms(uint64_t milliseconds) {
    if (cpu_cycles_per_ms > 0) {
        uint64_t cycles_to_wait = milliseconds * cpu_cycles_per_ms;
        uint64_t start = read_tsc();
        uint64_t target = start + cycles_to_wait;
        
        while (read_tsc() < target) {
            __asm__ volatile ("pause");
        }
    } else {
        volatile uint64_t counter = 0;
        while (counter < milliseconds * 10000) {
            counter++;
        }
        system_ticks += milliseconds;
    }
}

void format_uptime(char* buffer, int buffer_size) {
    if (buffer_size < 8) return;
    
    uint64_t uptime_ms = get_uptime_ms();
    uint64_t seconds = uptime_ms / 1000;
    
    if (seconds < 10) {
        buffer[0] = '0' + seconds;
        buffer[1] = 's';
        buffer[2] = '\0';
    } else {
        buffer[0] = '0' + (seconds / 10);
        buffer[1] = '0' + (seconds % 10);
        buffer[2] = 's';
        buffer[3] = '\0';
    }
}

void tick_increment(void) {
    system_ticks++;
}
