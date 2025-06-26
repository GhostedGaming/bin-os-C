#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Initialize CPUID system
void init_cpuid(void);

// Get CPU frequency in MHz
uint32_t get_cpu_frequency_mhz(void);

// Get formatted CPU info strings
void get_cpu_info_string(char* buffer, size_t size);
void get_cpu_speed_string(char* buffer, size_t size);

#endif