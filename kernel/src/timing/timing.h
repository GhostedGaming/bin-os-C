#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

// Initialize the timing system
void init_timing(void);

// Get uptime in milliseconds
uint64_t get_uptime_ms(void);

// Sleep for specified milliseconds
void sleep_ms(uint64_t milliseconds);

// Format uptime into a string
void format_uptime(char* buffer, int buffer_size);

// Internal function to increment ticks (called from timer interrupt)
void tick_increment(void);

#endif // TIMING_H
