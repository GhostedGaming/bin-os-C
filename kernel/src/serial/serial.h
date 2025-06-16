#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

int init_serial(void);
void write_serial(const char *str);

#endif // SERIAL_H
