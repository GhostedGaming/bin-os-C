#ifndef PRINT_VENDOR_H
#define PRINT_VENDOR_H

// CPU information structure
typedef struct {
    char* vendor_name;
    unsigned int base_freq_mhz;
    unsigned int max_freq_mhz;
    unsigned int bus_freq_mhz;
} cpu_info_t;

// Assembly functions (from cpuid_check.asm)
extern int check_cpu_and_get_vendor(void);
extern const char* get_cpu_vendor_string(void);
extern unsigned int get_cpu_speed_mhz(void);
extern void get_cpu_frequencies(void); // Returns in EAX, EBX, ECX

// C functions (from print_vendor.c)
cpu_info_t get_cpu_info(void);
char* process_cpu_vendor(void);
unsigned int get_cpu_base_frequency(void);
void get_detailed_cpu_frequencies(unsigned int* base_freq, unsigned int* max_freq, unsigned int* bus_freq);
void print_cpu_info(void);

#endif // PRINT_VENDOR_H