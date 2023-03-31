#include <kernel/pci.h>

#include <kernel/arch/i386/ports.h>

uint32_t pci_read_register(pci_device_t dev, uint8_t offset, uint8_t sz) {
    outportl(PCI_ADDRESS_PORT, pci_get_addr(dev, offset));

    uint32_t t = 0xffff;
    switch(sz) {
    case 4:
        t = inportl(PCI_VALUE_PORT);
        break;
    case 2:
        t = inportw(PCI_VALUE_PORT + (offset & 2));
        break;
    case 1:
        t = inportb(PCI_VALUE_PORT + (offset & 3));
        break;
    }
    return t;
}

void pci_write_register(pci_device_t dev, uint8_t offset, __unused uint8_t sz, uint32_t value) {
    outportl(PCI_ADDRESS_PORT, pci_get_addr(dev, offset));
    outportl(PCI_VALUE_PORT, value);
}

uint16_t pci_find_type(pci_device_t dev) {
    return (pci_read_register(dev, PCI_CLASS, 1) << 8)
         | (pci_read_register(dev, PCI_SUBCLASS, 1));
}

static void pci_scan_hit(pci_func_t f, pci_device_t dev, void *extra) {
    uint16_t vendor_id = (uint16_t)pci_read_register(dev, PCI_VENDOR_ID, 2);
    uint16_t device_id = (uint16_t)pci_read_register(dev, PCI_DEVICE_ID, 2);

    f(dev, vendor_id, device_id, extra);
}

void pci_scan_func(pci_func_t f, int32_t type, uint8_t bus, uint8_t slot, uint8_t func, void *extra) {
    pci_device_t dev = pci_get_device(bus, slot, func);
    uint16_t dev_type = pci_find_type(dev);

    if(type == -1 || type == dev_type)
        pci_scan_hit(f, dev, extra);
    if(dev_type == PCI_TYPE_BRIDGE)
        pci_scan_bus(f, type, pci_read_register(dev, PCI_SECONDARY_BUS, 1), extra);
}

void pci_scan_slot(pci_func_t f, int32_t type, uint8_t bus, uint8_t slot, void *extra) {
    pci_device_t dev = pci_get_device(bus, slot, 0);

    if(pci_read_register(dev, PCI_VENDOR_ID, 2) == PCI_NONE) return;
    pci_scan_func(f, type, bus, slot, 0, extra);
    if(!pci_read_register(dev, PCI_HEADER_TYPE, 1)) return;
    for(uint8_t func = 1; func < 8; func++) {
        dev = pci_get_device(bus, slot, func);
        if(pci_read_register(dev, PCI_VENDOR_ID, 2) != PCI_NONE)
            pci_scan_func(f, type, bus, slot, func, extra);
    }
}

void pci_scan_bus(pci_func_t f, int32_t type, uint8_t bus, void *extra) {
    for(uint8_t slot = 0; slot < 32; slot++)
        pci_scan_slot(f, type, bus, slot, extra);
}

/* Enumerate the PCI bus recursively, calling f on each matching device. */
void pci_scan(pci_func_t f, int32_t type, void *extra) {
    pci_device_t dev = pci_get_device(0, 0, 0);
    if(!(pci_read_register(dev, PCI_HEADER_TYPE, 1) & 0x80)) {
        pci_scan_bus(f, type, 0, extra);
        return;
    }

    uint8_t hit = 0;
    for(uint8_t func = 0; func < 8; func++) {
        dev = pci_get_device(0, 0, func);
        if(pci_read_register(dev, PCI_VENDOR_ID, 2) != PCI_NONE) {
            hit = 1;
            pci_scan_bus(f, type, func, extra);
        } else break;
    }
    if(hit) return;


    for(uint16_t bus = 0; bus < 256; bus++)
        for(uint8_t slot = 0; slot < 32; slot++)
            pci_scan_slot(f, type, bus, slot, extra);
}
