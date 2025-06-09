#include <stddef.h>
#include "print_vendor.h"

// Simple string comparison for kernel use
static int my_strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

// Function to get detailed CPU information
cpu_info_t get_cpu_info(void) {
    cpu_info_t info = {0};
    
    int cpuid_supported = check_cpu_and_get_vendor();
    
    if (!cpuid_supported) {
        info.vendor_name = "CPUID instruction not supported on this CPU";
        return info;
    }
    
    const char* vendor_string = get_cpu_vendor_string();
    
    // Determine vendor
    if (my_strncmp(vendor_string, "GenuineIntel", 12) == 0) {
        info.vendor_name = "Intel";
    } else if (my_strncmp(vendor_string, "AuthenticAMD", 12) == 0) {
        info.vendor_name = "AMD";
    } else if (my_strncmp(vendor_string, "CentaurHauls", 12) == 0) {
        info.vendor_name = "VIA/Centaur";
    } else {
        info.vendor_name = "Unknown or other processor vendor";
    }
    
    // Get CPU speed information
    info.base_freq_mhz = get_cpu_speed_mhz();
    
    // Get detailed frequency information
    // Note: This requires inline assembly or a separate assembly function
    // For now, we'll use the base frequency function
    
    return info;
}

// Original function for backward compatibility
char* process_cpu_vendor(void) {
    cpu_info_t info = get_cpu_info();
    return info.vendor_name;
}

// New function to get CPU speed
unsigned int get_cpu_base_frequency(void) {
    return get_cpu_speed_mhz();
}

// Function to get detailed frequency information
// This would typically use inline assembly to call the get_cpu_frequencies function
void get_detailed_cpu_frequencies(unsigned int* base_freq, unsigned int* max_freq, unsigned int* bus_freq) {
    // This requires inline assembly to call the assembly function
    // For demonstration, we'll use a simplified approach
    
    if (base_freq) *base_freq = get_cpu_speed_mhz();
    if (max_freq) *max_freq = 0;    // Would need assembly call to get this
    if (bus_freq) *bus_freq = 0;    // Would need assembly call to get this
    
    // Example of how you'd call the assembly function with inline assembly:
    /*
    unsigned int base, max, bus;
    __asm__ volatile (
        "call get_cpu_frequencies"
        : "=a" (base), "=b" (max), "=c" (bus)
        :
        : "memory"
    );
    
    if (base_freq) *base_freq = base;
    if (max_freq) *max_freq = max;
    if (bus_freq) *bus_freq = bus;
    */
}
