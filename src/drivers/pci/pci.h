#pragma once

#include <arch/io.h>
#include <list.h>
#include <types.h>
#include <utils/assert.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_COMMON_VENDOR_ID       0x00u
#define PCI_COMMON_DEVICE_ID       0x02u
#define PCI_COMMON_COMMAND         0x04u
#define PCI_COMMON_STATUS          0x06u
#define PCI_COMMON_REVISION_ID     0x08u
#define PCI_COMMON_PROG_IF         0x09u
#define PCI_COMMON_SUBCLASS        0x0Au
#define PCI_COMMON_CLASS_CODE      0x0Bu
#define PCI_COMMON_CACHE_LINE_SIZE 0x0Cu
#define PCI_COMMON_LATENCY_TIMER   0x0Du
#define PCI_COMMON_HEADER_TYPE     0x0Eu
#define PCI_COMMON_BIST            0x0Fu

#define PCI_GENERAL_BASE_ADDRESS_0       0x10u
#define PCI_GENERAL_BASE_ADDRESS_1       0x14u
#define PCI_GENERAL_BASE_ADDRESS_2       0x18u
#define PCI_GENERAL_BASE_ADDRESS_3       0x1Cu
#define PCI_GENERAL_BASE_ADDRESS_4       0x20u
#define PCI_GENERAL_BASE_ADDRESS_5       0x24u
#define PCI_GENERAL_CARDBUS_CIS_POINTER  0x28u
#define PCI_GENERAL_SUBSYSTEM_VENDOR_ID  0x2Cu
#define PCI_GENERAL_SUBSYSTEM_ID         0x2Eu
#define PCI_GENERAL_EXPANSION_ROM_BASE   0x30u
#define PCI_GENERAL_CAPABILITIES_POINTER 0x38u
#define PCI_GENERAL_INTERRUPT_LINE       0x3Cu
#define PCI_GENERAL_INTERRUPT_PIN        0x3Du
#define PCI_GENERAL_MIN_GRANT            0x3Eu
#define PCI_GENERAL_MAX_LATENCY          0x3Fu

#define PCI_VENDOR_ID_INVALID 0xFFFFu

#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80u
#define PCI_HEADER_TYPE_MASK          0x7Fu

#define PCI_MAX_BUS  256
#define PCI_MAX_SLOT 32
#define PCI_MAX_FUNC 8

#define PCI_COMMAND_IO_SPACE     0x0001u
#define PCI_COMMAND_MEMORY_SPACE 0x0002u
#define PCI_COMMAND_BUS_MASTER   0x0004u
#define PCI_COMMAND_SPECIAL      0x0008u
#define PCI_COMMAND_INVALIDATE   0x0010u
#define PCI_COMMAND_VGA_PALETTE  0x0020u
#define PCI_COMMAND_PARITY       0x0040u
#define PCI_COMMAND_SERR         0x0080u
#define PCI_COMMAND_FAST_BACK    0x0100u

#define PCI_STATUS_CAP_LIST       0x0080u
#define PCI_STATUS_66MHZ          0x0100u
#define PCI_STATUS_UDF            0x0200u
#define PCI_STATUS_DEVSEL_MASK    0x0600u
#define PCI_STATUS_DEVSEL         (PCI_STATUS_DEVSEL_MASK)
#define PCI_STATUS_SIGNAL_DETECT  0x0400u
#define PCI_STATUS_PARITY         0x0800u
#define PCI_STATUS_WAITING        0x1000u
#define PCI_STATUS_SERR           0x2000u
#define PCI_STATUS_CAP_LIST_ERROR 0x4000u

#define pci_log(dev, msg, ...) log("[%x:%x.%x] " msg, dev->bus, dev->slot, dev->func, ##__VA_ARGS__)

enum pci_device_type {
	PCI_DEVICE_UNKNOWN             = 0x0000,
	PCI_DEVICE_HOST_BRIDGE         = 0x0600,
	PCI_DEVICE_ISA_BRIDGE          = 0x0601,
	PCI_DEVICE_IDE_CONTROLLER      = 0x0101,
	PCI_DEVICE_VGA_CONTROLLER      = 0x0300,
	PCI_DEVICE_ETHERNET_CONTROLLER = 0x0200,
};

struct pci_device {
	uint8_t  bus, slot, func;
	uint16_t vendor_id, device_id;
	union {
		uint16_t type;
		struct {
			uint8_t subclass;
			uint8_t class_code;
		};
	};
	uint8_t          prog_if;
	struct list_head node;
};

static __always_inline uint32_t pci_config_read_dword(const struct pci_device *dev, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | offset | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	return inl(PCI_CONFIG_DATA);
}

static __always_inline uint16_t pci_config_read_word(const struct pci_device *dev, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFE) | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	return inw(PCI_CONFIG_DATA + (offset & 1));
}

static __always_inline uint8_t pci_config_read_byte(const struct pci_device *dev, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	return inb(PCI_CONFIG_DATA + (offset & 3));
}

static __always_inline void pci_config_write_dword(const struct pci_device *dev, uint8_t offset,
                                                   uint32_t value)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | offset | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	outl(PCI_CONFIG_DATA, value);
}

static __always_inline void pci_config_write_word(const struct pci_device *dev, uint8_t offset,
                                                  uint16_t value)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFE) | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	outw(PCI_CONFIG_DATA + (offset & 1), value);
}

static __always_inline void pci_config_write_byte(const struct pci_device *dev, uint8_t offset,
                                                  uint8_t value)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)dev->bus;
	uint32_t lslot = (uint32_t)dev->slot;
	uint32_t lfunc = (uint32_t)dev->func;

	address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (1u << 31);

	outl(PCI_CONFIG_ADDRESS, address);
	outb(PCI_CONFIG_DATA + (offset & 3), value);
}

static __always_inline void pci_enable_io_space(const struct pci_device *dev)
{
	uint16_t cmd = pci_config_read_word(dev, PCI_COMMON_COMMAND);
	if (!(cmd & PCI_COMMAND_IO_SPACE)) {
		cmd |= PCI_COMMAND_IO_SPACE;
		pci_config_write_word(dev, PCI_COMMON_COMMAND, cmd);
	}
}

void pci_init(void);
void pci_for_each_device(enum pci_device_type type,
                         void (*fn)(const struct pci_device *dev, void *ctx), void *ctx);
