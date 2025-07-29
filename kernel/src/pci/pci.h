#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint16_t pci_config_readWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

#endif // PCI_H