#ifndef PRINT_VENDOR_H
#define PRINT_VENDOR_H

// Assembly function declarations
extern int check_cpu_and_get_vendor(void);
extern const char* get_cpu_vendor_string(void);
extern unsigned int get_cpu_speed_mhz(void);
extern void get_cpu_frequencies(void); // Returns in EAX, EBX, ECX

// C function declarations
char* process_cpu_vendor(void);                    // Original function - returns vendor name
unsigned int get_cpu_base_frequency(void);         // Returns base CPU frequency in MHz
void get_detailed_cpu_frequencies(unsigned int* base_freq, 
                                 unsigned int* max_freq, 
                                 unsigned int* bus_freq);  // Gets all frequency info
void print_cpu_info(void);                         // Prints complete CPU information

// Structure for CPU information (if you want to use it externally)
typedef struct {
    char* vendor_name;
    unsigned int base_freq_mhz;
    unsigned int max_freq_mhz;
    unsigned int bus_freq_mhz;
} cpu_info_t;

// Function to get complete CPU info
cpu_info_t get_cpu_info(void);

#endif // PRINT_VENDOR_H