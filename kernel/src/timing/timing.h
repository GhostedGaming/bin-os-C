#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

void init_timing(void);
uint64_t get_uptime_ms(void);
void sleep_ms(uint64_t milliseconds);
void format_uptime(char* buffer, int buffer_size);
void tick_increment(void);

#endif