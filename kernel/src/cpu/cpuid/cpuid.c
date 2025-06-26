#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cpuid.h"
#include "../../serial/serial.h"

static uint32_t cpu_base_freq_mhz = 0;
static uint32_t cpu_max_freq_mhz = 0;

// External assembly functions
extern uint32_t get_cpu_base_frequency_mhz(void);
extern uint32_t get_cpu_max_frequency_mhz(void);

void init_cpuid(void) {
    // Get frequencies from assembly
    cpu_base_freq_mhz = get_cpu_base_frequency_mhz();
    cpu_max_freq_mhz = get_cpu_max_frequency_mhz();
    
    write_serial("CPUID initialized");
}

uint32_t get_cpu_frequency_mhz(void) {
    if (cpu_base_freq_mhz > 0) {
        return cpu_base_freq_mhz;
    }
    return cpu_max_freq_mhz;
}

void get_cpu_info_string(char* buffer, size_t size) {
    if (size < 20) return;
    
    const char* msg = "CPU: Detected";
    size_t i = 0;
    while (*msg && i < size - 1) {
        buffer[i++] = *msg++;
    }
    buffer[i] = '\0';
}

void get_cpu_speed_string(char* buffer, size_t size) {
    if (size < 20) return;
    
    uint32_t freq = get_cpu_frequency_mhz();
    if (freq > 0) {
        const char* prefix = "Speed: ";
        size_t i = 0;
        
        // Copy prefix
        while (*prefix && i < size - 1) {
            buffer[i++] = *prefix++;
        }
        
        // Simple number to string conversion
        char num_str[16];
        int num_len = 0;
        uint32_t temp = freq;
        
        if (temp == 0) {
            num_str[num_len++] = '0';
        } else {
            char temp_buf[16];
            int temp_pos = 0;
            while (temp > 0) {
                temp_buf[temp_pos++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int j = temp_pos - 1; j >= 0; j--) {
                num_str[num_len++] = temp_buf[j];
            }
        }
        num_str[num_len] = '\0';
        
        // Copy number
        int j = 0;
        while (num_str[j] && i < size - 5) {
            buffer[i++] = num_str[j++];
        }
        
        // Add " MHz"
        const char* suffix = " MHz";
        while (*suffix && i < size - 1) {
            buffer[i++] = *suffix++;
        }
        
        buffer[i] = '\0';
    } else {
        const char* msg = "Speed: Unknown";
        size_t i = 0;
        while (*msg && i < size - 1) {
            buffer[i++] = *msg++;
        }
        buffer[i] = '\0';
    }
}