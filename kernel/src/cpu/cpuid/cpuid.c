#include "cpuid.h"
#include "../../serial/serial.h"

extern char *get_vendor(void);

char *print_vendor() {
    char *vendor = get_vendor();
    serial_printf("Vendor: %s", vendor);
    return vendor;
}