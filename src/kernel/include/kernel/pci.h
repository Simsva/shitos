#ifndef KERNEL_PCI_H_
#define KERNEL_PCI_H_

#include <stdint.h>

#define PCI_VENDOR_ID            0x00 // 2
#define PCI_DEVICE_ID            0x02 // 2
#define PCI_COMMAND              0x04 // 2
#define PCI_STATUS               0x06 // 2
#define PCI_REVISION_ID          0x08 // 1

#define PCI_PROG_IF              0x09 // 1
#define PCI_SUBCLASS             0x0A // 1
#define PCI_CLASS                0x0B // 1
#define PCI_CACHE_LINE_SIZE      0x0C // 1
#define PCI_LATENCY_TIMER        0x0D // 1
#define PCI_HEADER_TYPE          0x0E // 1
#define PCI_BIST                 0x0F // 1
#define PCI_BAR0                 0x10 // 4
#define PCI_BAR1                 0x14 // 4
#define PCI_BAR2                 0x18 // 4
#define PCI_BAR3                 0x1c // 4
#define PCI_BAR4                 0x20 // 4
#define PCI_BAR5                 0x24 // 4

#define PCI_INTERRUPT_LINE       0x3c // 1
#define PCI_INTERRUPT_PIN        0x3d

#define PCI_SECONDARY_BUS        0x19 // 1

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2

#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106

#define PCI_ADDRESS_PORT 0xcf8
#define PCI_VALUE_PORT   0xcfc

#define PCI_NONE 0xffff

/* container type for PCI bus, slot, and function */
typedef union pci_device {
    uint32_t raw;
    struct {
        uint32_t offset : 8;
        uint32_t func   : 3;
        uint32_t slot   : 5;
        uint32_t bus    : 8;
        uint32_t _pad   : 8;
    };
} pci_device_t;

typedef void (*pci_func_t)(pci_device_t dev, uint16_t vendor_id, uint16_t device_id, void *extra);

static inline uint32_t pci_get_addr(pci_device_t dev, uint8_t offset) {
    dev.offset = offset & 0xfc;
    return UINT32_C(0x80000000) | dev.raw;
}

static inline pci_device_t pci_get_device(uint8_t bus, uint8_t slot, uint8_t func) {
    return (pci_device_t){ .bus = bus, .slot = slot, .func = func };
}

uint32_t pci_read_register(pci_device_t dev, uint8_t offset, uint8_t sz);
void pci_write_register(pci_device_t dev, uint8_t offset, uint8_t sz, uint32_t value);
uint16_t pci_find_type(pci_device_t dev);

void pci_scan_func(pci_func_t f, int32_t type, uint8_t bus, uint8_t slot, uint8_t func, void *extra);
void pci_scan_slot(pci_func_t f, int32_t type, uint8_t bus, uint8_t slot, void *extra);
void pci_scan_bus(pci_func_t f, int32_t type, uint8_t bus, void *extra);
void pci_scan(pci_func_t f, int32_t type, void *extra);

#endif // KERNEL_PCI_H_
