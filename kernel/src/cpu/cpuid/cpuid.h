#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Existing functions
bool cpuid_is_supported(void);
char* get_cpu_vendor(void);
char* get_cpu_vendor_c(void);
void get_cpu_vendor_safe(char* buffer, size_t buffer_size);
unsigned int get_cpu_max_function(void);
bool cpu_has_sse(void);
bool cpu_has_sse2(void);
void get_cpu_features(unsigned int* ecx_features, unsigned int* edx_features);

// New frequency detection functions
uint32_t get_cpu_base_frequency_mhz(void);
uint32_t get_cpu_max_frequency_mhz(void);
uint32_t get_tsc_frequency_mhz(void);
bool cpu_has_invariant_tsc(void);
bool cpu_supports_frequency_info(void);

#endif