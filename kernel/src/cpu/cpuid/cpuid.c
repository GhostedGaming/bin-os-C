#include <stdint.h>
#include <stddef.h>
#include "cpuid.h"

// Static buffer to hold vendor string
static char vendor_buffer[13];

// Assembly function to check if CPUID is supported
static int check_cpuid_support(void) {
    uint64_t flags_before, flags_after;
    
    // Try to flip the ID bit (bit 21) in EFLAGS
    __asm__ volatile (
        "pushfq\n\t"                    // Save original flags
        "pushfq\n\t"                    // Save flags again
        "xorq $0x200000, (%%rsp)\n\t"   // Flip ID bit in saved flags
        "popfq\n\t"                     // Load modified flags
        "pushfq\n\t"                    // Save flags again
        "popq %0\n\t"                   // Get modified flags
        "popfq"                         // Restore original flags
        : "=r" (flags_after)
        :
        : "memory"
    );
    
    __asm__ volatile (
        "pushfq\n\t"
        "popq %0"
        : "=r" (flags_before)
        :
        : "memory"
    );
    
    // If we can flip the ID bit, CPUID is supported
    return ((flags_before ^ flags_after) & 0x200000) != 0;
}

// Assembly function to execute CPUID instruction
static void execute_cpuid(uint32_t function, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (function)
    );
}

// Assembly function to execute CPUID with sub-function
static void execute_cpuid_sub(uint32_t function, uint32_t subfunction, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (function), "c" (subfunction)
    );
}

// Read Time Stamp Counter
static uint64_t read_tsc(void) {
    uint32_t low, high;
    __asm__ volatile (
        "rdtsc"
        : "=a" (low), "=d" (high)
    );
    return ((uint64_t)high << 32) | low;
}

int cpuid_is_supported(void) {
    return check_cpuid_support();
}

char* get_cpu_vendor(void) {
    if (!cpuid_is_supported()) {
        vendor_buffer[0] = '\0';
        return vendor_buffer;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0, &eax, &ebx, &ecx, &edx);
    
    // Store vendor string: EBX, EDX, ECX
    *((uint32_t*)&vendor_buffer[0]) = ebx;
    *((uint32_t*)&vendor_buffer[4]) = edx;
    *((uint32_t*)&vendor_buffer[8]) = ecx;
    vendor_buffer[12] = '\0';
    
    return vendor_buffer;
}

char* get_cpu_vendor_c(void) {
    return get_cpu_vendor();
}

void get_cpu_vendor_safe(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 13) {
        return;
    }
    
    char* vendor = get_cpu_vendor();
    for (size_t i = 0; i < 12 && i < buffer_size - 1; i++) {
        buffer[i] = vendor[i];
    }
    buffer[buffer_size - 1] = '\0';
}

unsigned int get_cpu_max_function(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0, &eax, &ebx, &ecx, &edx);
    return eax;
}

int cpu_has_sse(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(1, &eax, &ebx, &ecx, &edx);
    
    // SSE is bit 25 in EDX
    return (edx & (1 << 25)) ? 1 : 0;
}

int cpu_has_sse2(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(1, &eax, &ebx, &ecx, &edx);
    
    // SSE2 is bit 26 in EDX
    return (edx & (1 << 26)) ? 1 : 0;
}

void get_cpu_features(unsigned int* ecx_features, unsigned int* edx_features) {
    if (!ecx_features || !edx_features) {
        return;
    }
    
    if (!cpuid_is_supported()) {
        *ecx_features = 0;
        *edx_features = 0;
        return;
    }
    
    uint32_t eax, ebx;
    execute_cpuid(1, &eax, &ebx, ecx_features, edx_features);
}

// Get CPU base frequency from CPUID (Intel only, function 0x16)
uint32_t get_cpu_base_frequency_mhz(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    // Check if function 0x16 is supported
    if (get_cpu_max_function() < 0x16) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0x16, &eax, &ebx, &ecx, &edx);
    
    // EAX contains base frequency in MHz
    return eax & 0xFFFF;
}

// Get CPU maximum frequency from CPUID (Intel only, function 0x16)
uint32_t get_cpu_max_frequency_mhz(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    // Check if function 0x16 is supported
    if (get_cpu_max_function() < 0x16) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0x16, &eax, &ebx, &ecx, &edx);
    
    // EBX contains maximum frequency in MHz
    return ebx & 0xFFFF;
}

// Get TSC frequency from CPUID (Intel only, function 0x15)
uint64_t get_tsc_frequency_hz(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    // Check if function 0x15 is supported
    if (get_cpu_max_function() < 0x15) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0x15, &eax, &ebx, &ecx, &edx);
    
    // EAX = denominator, EBX = numerator, ECX = crystal frequency
    if (eax == 0 || ebx == 0) {
        return 0;
    }
    
    // If crystal frequency is provided
    if (ecx != 0) {
        return ((uint64_t)ecx * ebx) / eax;
    }
    
    // Fallback: try to get base frequency and calculate
    uint32_t base_freq = get_cpu_base_frequency_mhz();
    if (base_freq != 0) {
        return ((uint64_t)base_freq * 1000000ULL * ebx) / eax;
    }
    
    return 0;
}

// Measure CPU frequency using TSC and a known timer (requires PIT or other timer)
// This is a fallback method when CPUID doesn't provide frequency info
uint64_t measure_cpu_frequency_hz(void (*delay_ms_func)(uint32_t)) {
    if (!delay_ms_func) {
        return 0;
    }
    
    // Measure TSC over 100ms
    uint64_t tsc_start = read_tsc();
    delay_ms_func(100);  // 100ms delay using your existing timer
    uint64_t tsc_end = read_tsc();
    
    // Calculate frequency: (tsc_diff / 0.1 seconds)
    uint64_t tsc_diff = tsc_end - tsc_start;
    return tsc_diff * 10;  // * 10 because we measured over 0.1 seconds
}

// Get the best available CPU frequency
uint64_t get_cpu_frequency_hz(void (*delay_ms_func)(uint32_t)) {
    uint64_t freq = 0;
    
    // Try TSC frequency first (most accurate)
    freq = get_tsc_frequency_hz();
    if (freq != 0) {
        return freq;
    }
    
    // Try base frequency from CPUID
    uint32_t base_freq_mhz = get_cpu_base_frequency_mhz();
    if (base_freq_mhz != 0) {
        return (uint64_t)base_freq_mhz * 1000000ULL;
    }
    
    // Fallback: measure using existing timer
    if (delay_ms_func) {
        return measure_cpu_frequency_hz(delay_ms_func);
    }
    
    return 0;
}

// Check if TSC is invariant (doesn't change with CPU frequency scaling)
int cpu_has_invariant_tsc(void) {
    if (!cpuid_is_supported()) {
        return 0;
    }
    
    // Check if extended function 0x80000007 is supported
    uint32_t eax, ebx, ecx, edx;
    execute_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000007) {
        return 0;
    }
    
    execute_cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
    
    // Invariant TSC is bit 8 in EDX
    return (edx & (1 << 8)) ? 1 : 0;
}
