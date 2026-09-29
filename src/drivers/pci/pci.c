#include "pci.h"
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

/* non-static because of the debug function `pci_list_devices` (see `src/drivers/tty.c`) */
/* static */ LIST_HEAD(pci_devices);

static __always_inline uint16_t get_vendor_id(const struct pci_device *dev)
{
	return pci_config_read_word(dev, PCI_COMMON_VENDOR_ID);
}

static __always_inline uint16_t get_device_id(const struct pci_device *dev)
{
	return pci_config_read_word(dev, PCI_COMMON_DEVICE_ID);
}

static __always_inline uint8_t get_class(const struct pci_device *dev)
{
	return pci_config_read_byte(dev, PCI_COMMON_CLASS_CODE);
}

static __always_inline uint8_t get_subclass(const struct pci_device *dev)
{
	return pci_config_read_byte(dev, PCI_COMMON_SUBCLASS);
}

static __always_inline uint8_t get_prog_if(const struct pci_device *dev)
{
	return pci_config_read_byte(dev, PCI_COMMON_PROG_IF);
}

static __always_inline uint8_t get_header_type(const struct pci_device *dev)
{
	return pci_config_read_byte(dev, PCI_COMMON_HEADER_TYPE);
}

static struct pci_device *pci_register_device(const struct pci_device *probe)
{
	uint8_t class_code = get_class(probe);
	uint8_t subclass   = get_subclass(probe);

	struct pci_device *dev = kmalloc(sizeof(struct pci_device), GFP_KERNEL);

	if (!dev) {
		pci_log(probe, "Failed to allocate `pci_device', registry entry skipped.");
		return NULL;
	}

	dev->bus        = probe->bus;
	dev->slot       = probe->slot;
	dev->func       = probe->func;
	dev->vendor_id  = get_vendor_id(dev);
	dev->device_id  = get_device_id(dev);
	dev->prog_if    = get_prog_if(dev);
	dev->class_code = class_code;
	dev->subclass   = subclass;

	list_add_tail(&dev->node, &pci_devices);
	return dev;
}

void pci_for_each_device(enum pci_device_type type,
                         void (*fn)(const struct pci_device *dev, void *ctx), void *ctx)
{
	struct pci_device *dev;
	list_for_each_entry(dev, &pci_devices, node)
	{
		if (dev->type == type)
			fn(dev, ctx);
	}
}

void pci_init(void)
{
	struct pci_device probe;

	probe.bus = 0;
	do {
		for (probe.slot = 0; probe.slot < PCI_MAX_SLOT; probe.slot++) {
			probe.func = 0;

			if (get_vendor_id(&probe) == PCI_VENDOR_ID_INVALID)
				continue;

			struct pci_device *dev = pci_register_device(&probe);
			if (!dev)
				continue;

			if (!(get_header_type(dev) & PCI_HEADER_TYPE_MULTIFUNCTION))
				continue;

			for (probe.func = 1; probe.func < PCI_MAX_FUNC; probe.func++) {
				if (get_vendor_id(&probe) == PCI_VENDOR_ID_INVALID)
					continue;

				struct pci_device *fdev = pci_register_device(&probe);
				if (!fdev)
					continue;
			}
		}
	} while (++probe.bus != 0); /* uint8_t overflow after PCI_MAX_BUS (256 buses) */
}
