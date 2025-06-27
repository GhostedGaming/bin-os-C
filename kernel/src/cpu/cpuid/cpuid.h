#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool has_cpuid();
char *get_vendor_string();

#endif