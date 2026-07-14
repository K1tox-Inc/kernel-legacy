#pragma once

#include <arch/io.h>
#include <list.h>
#include <types.h>
#include <utils/assert.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

/*
 * These structures are basely not intended to be instantiated.
 * They are useful when being used with `offsetof` to use functions below,
 * but take care about using right functions for right types.
 */

struct pci_common_headers {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t  revision_id;
	uint8_t  prog_if;
	uint8_t  subclass;
	uint8_t  class_code;
	uint8_t  cache_line_size;
	uint8_t  latency_timer;
	uint8_t  header_type;
	uint8_t  bist;
} __packed;

struct pci_general_device_headers {
	struct pci_common_headers common;

	uintptr_t base_address_0;
	uintptr_t base_address_1;
	uintptr_t base_address_2;
	uintptr_t base_address_3;
	uintptr_t base_address_4;
	uintptr_t base_address_5;
	uintptr_t cardbus_cis_pointer;
	uint16_t  subsystem_vendor_id;
	uint16_t  subsystem_id;
	uintptr_t expansion_rom_base_address;
	uint8_t   capabilities_pointer;
	uint32_t : 24;
	uint32_t : 32;
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint8_t min_grant;
	uint8_t max_latency;
} __packed;

static_assert(offsetof(struct pci_common_headers, cache_line_size) == 0x0C,
              "Offset of `cache_line_size` in `struct pci_common_headers` should be "
              "equal to 0x0C");

static_assert(offsetof(struct pci_general_device_headers, subsystem_vendor_id) == 0x2C,
              "Offset of `subsystem_vendor_id` in `struct pci_general_device_headers` should be "
              "equal to 0x2C");

static_assert(offsetof(struct pci_common_headers, bist) ==
                  offsetof(struct pci_general_device_headers, common.bist),
              "Common headers should be aligned through inherit structs");

static __always_inline uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                                                      uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address =
	    (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | offset | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	return inl(0xCFC);
}

static __always_inline uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                                                     uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFE) |
	                     ((uint32_t)0x80000000));

	outl(0xCF8, address);
	return (inl(0xCFC) >> ((offset & 1) * 8)) & 0xFFFF;
}

static __always_inline uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func,
                                                    uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) |
	                     ((uint32_t)0x80000000));

	outl(0xCF8, address);
	return (inl(0xCFC) >> ((offset & 3) * 8)) & 0xFF;
}

/* Sentinel returned by a config-space read at a slot/function with no device present. */
#define PCI_VENDOR_ID_INVALID 0xFFFFu

/* Bit 7 of `header_type` marks a device as exposing more than one function. */
#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80u
#define PCI_HEADER_TYPE_MASK          0x7Fu

#define PCI_MAX_BUS  256
#define PCI_MAX_SLOT 32
#define PCI_MAX_FUNC 8

enum pci_device_type {
	PCI_DEVICE_UNKNOWN = 0,
	PCI_DEVICE_HOST_BRIDGE,
	PCI_DEVICE_ISA_BRIDGE,
	PCI_DEVICE_IDE_CONTROLLER,
	PCI_DEVICE_VGA_CONTROLLER,
	PCI_DEVICE_ETHERNET_CONTROLLER,
};

/*
 * A single enumerated PCI function, cached after the boot-time scan so
 * drivers (e.g. the IDE/ATA driver) don't need to re-walk config space to
 * find their device.
 */
struct pci_device {
	uint8_t              bus, slot, func;
	uint16_t             vendor_id, device_id;
	uint8_t              class_code, subclass, prog_if;
	enum pci_device_type type;
	struct list_head     node;
};

void pci_init(void);

/*
 * Returns the first registered device of the given type, or NULL if none
 * was found during the boot-time scan. Intended for drivers that need to
 * locate "their" device (e.g. the IDE driver looking up prog_if to decide
 * legacy vs. native mode) without re-walking PCI config space.
 */
struct pci_device *pci_find_device(enum pci_device_type type);
