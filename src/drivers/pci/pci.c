#include "pci.h"
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

/* static */ LIST_HEAD(pci_devices);

static __always_inline uint16_t get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_word(bus, slot, func, offsetof(struct pci_common_headers, vendor_id));
}

static __always_inline uint16_t get_device_id(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_word(bus, slot, func, offsetof(struct pci_common_headers, device_id));
}

static __always_inline uint8_t get_class(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_byte(bus, slot, func, offsetof(struct pci_common_headers, class_code));
}

static __always_inline uint8_t get_subclass(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_byte(bus, slot, func, offsetof(struct pci_common_headers, subclass));
}

static __always_inline uint8_t get_prog_if(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_byte(bus, slot, func, offsetof(struct pci_common_headers, prog_if));
}

static __always_inline uint8_t get_header_type(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pci_config_read_byte(bus, slot, func, offsetof(struct pci_common_headers, header_type));
}

/*
 * Allocates and fills a `struct pci_device` for the given function and
 * links it into the global registry. Re-reads the identifying fields
 * directly rather than trusting the caller, so the registry stays correct
 * even if callers are reorganized later.
 */
static struct pci_device *pci_register_device(uint8_t bus, uint8_t slot, uint8_t func,
                                              enum pci_device_type type)
{
	struct pci_device *dev = kmalloc(sizeof(struct pci_device), GFP_KERNEL);

	if (!dev)
		return NULL;

	dev->bus        = bus;
	dev->slot       = slot;
	dev->func       = func;
	dev->vendor_id  = get_vendor_id(bus, slot, func);
	dev->device_id  = get_device_id(bus, slot, func);
	dev->class_code = get_class(bus, slot, func);
	dev->subclass   = get_subclass(bus, slot, func);
	dev->prog_if    = get_prog_if(bus, slot, func);
	dev->type       = type;

	list_add_tail(&dev->node, &pci_devices);
	return dev;
}

/* Identifies, logs, and registers a single (bus, slot, func) PCI function. */
static void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint8_t              class_code = get_class(bus, slot, func);
	uint8_t              subclass   = get_subclass(bus, slot, func);
	enum pci_device_type type       = PCI_DEVICE_UNKNOWN;

	switch (class_code) {
	case 0x00: {
		switch (subclass) {
		case 0x00:
			log("[%x:%x.%x] Non-VGA-compatible unclassified device discovered.", bus, slot, func);
			break;
		default:
			log("[%x:%x.%x] Unknown unclassified device: subclass=0x%x", bus, slot, func, subclass);
		}
		break;
	}
	case 0x01: {
		switch (subclass) {
		case 0x01:
			log("[%x:%x.%x] IDE controller discovered (prog_if=0x%x).", bus, slot, func,
			    get_prog_if(bus, slot, func));
			type = PCI_DEVICE_IDE_CONTROLLER;
			break;
		default:
			log("[%x:%x.%x] Unknown mass storage controller: subclass=0x%x", bus, slot, func,
			    subclass);
		}
		break;
	}
	case 0x02: {
		switch (subclass) {
		case 0x00:
			log("[%x:%x.%x] Ethernet controller discovered.", bus, slot, func);
			type = PCI_DEVICE_ETHERNET_CONTROLLER;
			break;
		default:
			log("[%x:%x.%x] Unknown network controller: subclass=0x%x", bus, slot, func, subclass);
		}
		break;
	}
	case 0x03: {
		log("[%x:%x.%x] VGA compatible controller discovered.", bus, slot, func);
		type = PCI_DEVICE_VGA_CONTROLLER;
		break;
	}
	case 0x06: {
		switch (subclass) {
		case 0x00:
			log("[%x:%x.%x] Host bridge discovered.", bus, slot, func);
			type = PCI_DEVICE_HOST_BRIDGE;
			break;
		case 0x01:
			log("[%x:%x.%x] ISA bridge discovered.", bus, slot, func);
			type = PCI_DEVICE_ISA_BRIDGE;
			break;
		default:
			log("[%x:%x.%x] Unknown bridge device: subclass=0x%x", bus, slot, func, subclass);
		}
		break;
	}
	default:
		log("[%x:%x.%x] Unknown device class: class=0x%x subclass=0x%x", bus, slot, func,
		    class_code, subclass);
	}

	if (!pci_register_device(bus, slot, func, type))
		log("[%x:%x.%x] Failed to allocate pci_device, registry entry skipped.", bus, slot, func);
}

/*
 * Scans function 0 of a slot; if it reports itself as multi-function, scans
 * functions 1-7 too. Each function has to be probed independently since a
 * multi-function device is not required to populate every function slot.
 */
static void pci_scan_slot(uint8_t bus, uint8_t slot)
{
	if (get_vendor_id(bus, slot, 0) == PCI_VENDOR_ID_INVALID)
		return;

	pci_scan_function(bus, slot, 0);

	if (!(get_header_type(bus, slot, 0) & PCI_HEADER_TYPE_MULTIFUNCTION))
		return;

	for (uint8_t func = 1; func < PCI_MAX_FUNC; func++) {
		if (get_vendor_id(bus, slot, func) == PCI_VENDOR_ID_INVALID)
			continue;
		pci_scan_function(bus, slot, func);
	}
}

void pci_init(void)
{
	for (int bus = 0; bus < PCI_MAX_BUS; bus++)
		for (int slot = 0; slot < PCI_MAX_SLOT; slot++)
			pci_scan_slot((uint8_t)bus, (uint8_t)slot);
}

struct pci_device *pci_find_device(enum pci_device_type type)
{
	struct pci_device *dev;

	list_for_each_entry(dev, &pci_devices, node)
	{
		if (dev->type == type)
			return dev;
	}

	return NULL;
}
