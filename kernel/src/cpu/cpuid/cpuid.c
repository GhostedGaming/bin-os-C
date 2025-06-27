#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cpuid.h"
#include "../../serial/serial.h"

// External assembly functions
extern bool check_for_cpuid(void);
extern char *get_vendor(void);

bool has_cpuid() {
    return !check_for_cpuid();
}

char *get_vendor_string() {
    return get_vendor();
}