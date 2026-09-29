#include <arch/io.h>
#include <arch/x86.h>
#include <kernel/block.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <utils/assert.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

#include "ata.h"
#include "kernel/panic.h"
#include "types.h"

static LIST_HEAD(ide_channels);
LIST_HEAD(ide_devices);

void memory_dump(const uint8_t *addr_start, const uint8_t *addr_end);

static void ide_write_reg(const struct ide_channel *channel, uint8_t reg, uint8_t value)
{
	if (reg > 0x07 && reg < 0x0C)
		ide_write_reg(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);

	if (reg < 0x08)
		outb(channel->base_port + reg, value);
	else if (reg < 0x0C)
		outb(channel->base_port + reg - 0x06, value);
	else if (reg < 0x0E)
		outb(channel->control_port + reg - 0x0A, value);
	else if (reg >= 0x0E && channel->bmide)
		outb(channel->bmide + reg - 0x0E, value);

	if (reg > 0x07 && reg < 0x0C)
		ide_write_reg(channel, ATA_REG_CONTROL, channel->nIEN);
}

static uint8_t ide_read_reg(const struct ide_channel *channel, uint8_t reg)
{
	uint8_t result;

	if (reg > 0x07 && reg < 0x0C)
		ide_write_reg(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);

	if (reg < 0x08)
		result = inb(channel->base_port + reg);
	else if (reg < 0x0C)
		result = inb(channel->base_port + reg - 0x06);
	else if (reg < 0x0E)
		result = inb(channel->control_port + reg - 0x0A);
	else if (reg >= 0x0E && channel->bmide)
		result = inb(channel->bmide + reg - 0x0E);
	else
		result = 0xFF;

	if (reg > 0x07 && reg < 0x0C)
		ide_write_reg(channel, ATA_REG_CONTROL, channel->nIEN);

	return result;
}

uint8_t ide_polling(const struct ide_channel *channel, bool advanced_check)
{

	for (int i = 0; i < 4; i++)
		ide_read_reg(channel, ATA_REG_ALTSTATUS);

	while (ide_read_reg(channel, ATA_REG_STATUS) & ATA_SR_BSY)
		;

	if (advanced_check) {
		unsigned char state = ide_read_reg(channel, ATA_REG_STATUS);

		if (state & ATA_SR_DF)
			return 1;

		if (state & ATA_SR_ERR)
			return 2;

		if ((state & ATA_SR_DRQ) == 0)
			return 3;
	}

	return 0;
}

static void ide_id_extract_string(const uint16_t *buf, size_t byte_off, char *out, size_t max_len)
{
	size_t words = max_len / 2;
	size_t word  = byte_off / 2;
	size_t pos   = 0;

	for (size_t i = 0; i < words && pos + 1 < max_len; ++i) {
		uint16_t value = buf[word + i];

		out[pos++] = (char)(value >> 8);
		if (pos < max_len - 1)
			out[pos++] = (char)(value & 0xFF);
	}

	out[pos] = '\0';

	while (pos > 0 && (out[pos - 1] == ' ' || out[pos - 1] == '\0')) {
		out[--pos] = '\0';
	}
}

static void ide_probe_controller(const ide_controller_t *ctrlr, __always_unused void *ctx)
{
	uint32_t            bar[6];
	struct ide_channel *channels = kmalloc(sizeof(struct ide_channel) * 2, GFP_KERNEL);

	bar[0] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_0);
	bar[1] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_1);
	bar[2] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_2);
	bar[3] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_3);
	bar[4] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_4);
	bar[5] = pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_5);

	if (bar[0] == 0)
		bar[0] = ATA_PRIMARY_BASE;
	if (bar[1] == 0)
		bar[1] = ATA_PRIMARY_CONTROL;
	if (bar[2] == 0)
		bar[2] = ATA_SECONDARY_BASE;
	if (bar[3] == 0)
		bar[3] = ATA_SECONDARY_CONTROL;

	channels->base_port    = bar[0];
	channels->control_port = bar[1];
	channels->bmide        = bar[4];
	channels->nIEN         = 0x02;
	list_add_tail(&channels->node, &ide_channels);

	channels++;

	channels->base_port    = bar[2];
	channels->control_port = bar[3];
	channels->bmide        = bar[5];
	channels->nIEN         = 0x02;
	list_add_tail(&channels->node, &ide_channels);
}

static int ide_identify(const struct ide_channel *channel, const struct ide_device *dev,
                        uint16_t *buf)
{
	int     i;
	uint8_t st;

	for (i = 0; i < 1000; i++) {
		st = ide_read_reg(channel, ATA_REG_STATUS);
		if (!(st & ATA_SR_BSY))
			break;
	}

	if (st & ATA_SR_BSY) {
		return -1;
	}

	ide_write_reg(channel, ATA_REG_HDDEVSEL, (uint8_t)(0xA0 | (dev->slave << 4)));
	ide_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

	for (i = 0; i < 10000; i++) {
		st = ide_read_reg(channel, ATA_REG_STATUS);
		if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
			break;
		if (st & ATA_SR_ERR) {
			return -1;
		}
	}

	if (!(st & ATA_SR_DRQ)) {
		return -1;
	}

	insl(channel->base_port + ATA_REG_DATA, buf, 128);
	st = ide_read_reg(channel, ATA_REG_STATUS);
	if (st & ATA_SR_ERR) {
		return -1;
	}

	return 0;
}

static int ide_populate_drive(const struct ide_channel *channel, struct ide_device *dev)
{
	uint16_t id_buf[256];

	if (ide_identify(channel, dev, id_buf) != 0) {
		return -1;
	}

	dev->dev_type  = id_buf[ATA_IDENT_DEVICETYPE / 2];
	dev->cylinders = id_buf[ATA_IDENT_CYLINDERS / 2];
	dev->heads     = id_buf[ATA_IDENT_HEADS / 2];
	dev->sectors   = id_buf[ATA_IDENT_SECTORS / 2];
	dev->lba_max_sectors =
	    id_buf[ATA_IDENT_MAX_LBA / 2] | (id_buf[ATA_IDENT_MAX_LBA / 2 + 1] << 16);
	dev->capabilities = id_buf[ATA_IDENT_CAPABILITIES / 2];
	dev->field_valid  = id_buf[ATA_IDENT_FIELDVALID / 2];
	dev->lba_ext_max_sectors =
	    id_buf[ATA_IDENT_MAX_LBA_EXT / 2] | (id_buf[ATA_IDENT_MAX_LBA_EXT / 2 + 1] << 16);

	ide_id_extract_string(id_buf, ATA_IDENT_MODEL, dev->model, sizeof(dev->model));
	ide_id_extract_string(id_buf, ATA_IDENT_SERIAL, dev->serial, sizeof(dev->serial));

	return 0;
}

static void ide_probe_channels(void)
{
	struct ide_channel *channel;
	struct ide_device  *dev;

	list_for_each_entry(channel, &ide_channels, node)
	{
		int slave;

		for (slave = 0; slave < 2; slave++) {
			dev = kmalloc(sizeof(struct ide_device), GFP_KERNEL);
			if (!dev) {
				continue;
			}

			dev->channel = channel;
			dev->slave   = slave;
			dev->present = (ide_populate_drive(channel, dev) == 0);

			if (dev->present) {
				list_add_tail(&dev->node, &ide_devices);
			} else {
				kfree(dev);
			}
		}
	}
}

static uint8_t ata_access_sector(const struct ide_device *dev, uint32_t lba,
                                 enum ide_access_direction direction, uint8_t numsects, void *edi)
{
	uint8_t lba_io[6], head;

	ide_write_reg(dev->channel, ATA_REG_CONTROL, 0x02);

	if (lba >= 0x10000000) {
		kpanic("LBA48 not implemented");
	} else if (dev->capabilities & (1 << 9)) {
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head      = (lba & 0xF000000) >> 24;
	} else {
		kpanic("CHS not implemented");
	}

	while (ide_read_reg(dev->channel, ATA_REG_STATUS) & ATA_SR_BSY)
		;

	ide_write_reg(dev->channel, ATA_REG_HDDEVSEL, 0xE0 | (dev->slave << 4) | head);

	ide_write_reg(dev->channel, ATA_REG_SECCOUNT0, numsects);
	ide_write_reg(dev->channel, ATA_REG_LBA0, lba_io[0]);
	ide_write_reg(dev->channel, ATA_REG_LBA1, lba_io[1]);
	ide_write_reg(dev->channel, ATA_REG_LBA2, lba_io[2]);

	ide_write_reg(dev->channel, ATA_REG_COMMAND,
	              (direction == IDE_ACCESS_READ) ? ATA_CMD_READ_PIO : ATA_CMD_WRITE_PIO);

	uint8_t err;

	switch (direction) {
	case IDE_ACCESS_READ: {
		for (int i = 0; i < numsects; i++) {
			err = ide_polling(dev->channel, true);
			if (err)
				return err;
			__asm__ volatile("pushw %es");
			__asm__ volatile("mov %%ax, %%es"
			                 :
			                 : "a"(GDT_SELECTOR(GDT_IDX_KERNEL_DATA, KERNEL_RING)));
			__asm__ volatile("rep insw"
			                 :
			                 : "c"(ATA_SECTOR_SIZE / 2), "d"(dev->channel->base_port), "D"(edi));
			__asm__ volatile("popw %es");
			edi += ATA_SECTOR_SIZE;
		}
		break;
	}
	case IDE_ACCESS_WRITE: {
		for (int i = 0; i < numsects; i++) {
			ide_polling(dev->channel, false);
			__asm__ volatile("pushw %ds");
			__asm__ volatile(
			    "mov %%ax, %%ds" ::"a"(GDT_SELECTOR(GDT_IDX_KERNEL_DATA, KERNEL_RING)));
			__asm__ volatile("rep outsw" ::"c"(ATA_SECTOR_SIZE / 2), "d"(dev->channel->base_port),
			                 "S"(edi));
			__asm__ volatile("popw %ds");
			edi += ATA_SECTOR_SIZE;
		}
		ide_write_reg(dev->channel, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
		ide_polling(dev->channel, false);
		break;
	}
	default:
		kpanic("lmao you're so funny :)");
	}

	return 0;
}

static int ata_read_sector(struct generic_disk *disk, size_t lba, size_t numsects, void *edi)
{
	return ata_access_sector((struct ide_device *)disk->priv, lba, IDE_ACCESS_READ, numsects, edi);
}

static int ata_write_sector(struct generic_disk *disk, size_t lba, size_t numsects, const void *edi)
{
	return ata_access_sector((struct ide_device *)disk->priv, lba, IDE_ACCESS_WRITE, numsects,
	                         (void *)edi);
}

static const struct disk_ops ide_ops = (struct disk_ops){
    .read_sectors = &ata_read_sector, .write_sectors = &ata_write_sector, .flush = NULL};

void ide_init(void)
{
	pci_for_each_device(PCI_DEVICE_IDE_CONTROLLER, ide_probe_controller, NULL);
	ide_probe_channels();

	struct ide_device *dev;
	char               i = 0;
	list_for_each_entry(dev, &ide_devices, node)
	{
		struct generic_disk *disk = kmalloc(sizeof(struct generic_disk), GFP_KERNEL);
		if (disk == NULL) {
			log("Failed to allocate `struct generic_disk', skipping...");
			continue;
		}

		disk->priv       = dev;
		disk->ops        = &ide_ops;
		disk->nr_sectors = dev->sectors;

		ft_memcpy(disk->name, "hd0", 4);
		disk->name[2] += i++;

		if (add_disk(disk) < 0)
			kfree(disk);
	}

	/*
	 * Usage exemple:
	 * const struct ide_device *device;
	 *
	 * uint8_t   buffer[ATA_SECTOR_SIZE * 1];
	 * uintptr_t addr          = 0xd0d0;
	 * uint32_t  lba           = addr / ATA_SECTOR_SIZE;
	 * size_t    buffer_offset = addr % ATA_SECTOR_SIZE;
	 *
	 * device = list_next_entry(list_first_entry(&ide_devices, struct ide_device, node), node);
	 *
	 * ata_read_sector(device, lba, sizeof(buffer) / ATA_SECTOR_SIZE, buffer);
	 *
	 * ft_memcpy(buffer + buffer_offset, "Hello, World!", 13);
	 *
	 * ata_write_sector(device, lba, sizeof(buffer) / ATA_SECTOR_SIZE, buffer);
	 */
}
