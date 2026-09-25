#include <utils/assert.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>

#include "../../include/arch/io.h"
#include "ata.h"
#include "list.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"

#define ATA_PRIMARY_BASE      0x1F0
#define ATA_PRIMARY_CONTROL   0x3F6
#define ATA_SECONDARY_BASE    0x170
#define ATA_SECONDARY_CONTROL 0x376

static void ide_write_reg(struct ide_channel *channel, uint8_t reg, uint8_t value)
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

static uint8_t ide_read_reg(struct ide_channel *channel, uint8_t reg)
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

static LIST_HEAD(ide_channels);
LIST_HEAD(ide_devices);

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

#ifndef NDEBUG
static void ide_log_channels(void)
{
	struct ide_channel *ch;
	int                 n = 0;
	list_for_each_entry(ch, &ide_channels, node)
	{
		log("[ide][ctrl] channel[%d]: base=0x%04x ctrl=0x%04x bmide=0x%04x nIEN=0x%02x", n++,
		    ch->base_port, ch->control_port, ch->bmide, ch->nIEN);
	}
	if (n == 0)
		log("[ide][ctrl] NO IDE channels registered");
}
#endif

static void ide_probe_controller(const ide_controller_t *ctrlr, __always_unused void *ctx)
{
#define bar(x) pci_config_read_dword(ctrlr, PCI_GENERAL_BASE_ADDRESS_##x)

	uint32_t            bar0, bar1, bar2, bar3, bar4, bar5;
	int                 n_ctrls = 0;
	struct ide_channel *channels;

	bar0 = bar(0);
	bar1 = bar(1);
	bar2 = bar(2);
	bar3 = bar(3);
	bar4 = bar(4);
	bar5 = bar(5);

	log("[ide][ctrl] --- controller @ %02x:%02x.%02x ---", ctrlr->bus, ctrlr->slot, ctrlr->func);
	log("[ide][ctrl]   BAR0=0x%08x  BAR1=0x%08x  BAR2=0x%08x  BAR3=0x%08x", bar0, bar1, bar2, bar3);
	log("[ide][ctrl]   BAR4=0x%08x  BAR5=0x%08x", bar4, bar5);

	channels = kmalloc(sizeof(struct ide_channel) * 2, GFP_KERNEL);
	if (!channels) {
		pci_log(ctrlr, "Failed to allocate channels. Skipping...");
		return;
	}

	/*
	 * PIIX3 IDE in QEMU: BAR0-3 = 0 (no MMIO), legacy I/O ports are
	 * hardcoded at 0x1F0/0x3F6 (primary) and 0x170/0x376 (secondary).
	 * BAR4/5 (bmide, optional DMA window) may also be 0.
	 * Fall back to legacy ports when the BAR is zero.
	 */
	if (bar0 == 0)
		bar0 = ATA_PRIMARY_BASE;
	if (bar1 == 0)
		bar1 = ATA_PRIMARY_CONTROL;
	if (bar2 == 0)
		bar2 = ATA_SECONDARY_BASE;
	if (bar3 == 0)
		bar3 = ATA_SECONDARY_CONTROL;

	channels->base_port    = bar0;
	channels->control_port = bar1;
	channels->bmide        = bar4; /* bar4 may be 0 -> legacy nIEN path */
	channels->nIEN         = 0x02;
	list_add_tail(&channels->node, &ide_channels);
	log("[ide][ctrl]   [0] master: base=0x%04x ctrl=0x%04x bmide=0x%04x", channels->base_port,
	    channels->control_port, channels->bmide);
	n_ctrls++;

	channels++;

	channels->base_port    = bar2;
	channels->control_port = bar3;
	channels->bmide        = bar5;
	channels->nIEN         = 0x02;
	list_add_tail(&channels->node, &ide_channels);
	log("[ide][ctrl]   [1] slave : base=0x%04x ctrl=0x%04x bmide=0x%04x", channels->base_port,
	    channels->control_port, channels->bmide);
	n_ctrls++;

	log("[ide][ctrl]   registered %d channel(s) total", n_ctrls);

#ifndef NDEBUG
	ide_log_channels();
#endif

#undef bar
}

static int ide_identify(struct ide_channel *channel, struct ide_device *dev, uint16_t *buf)
{
	int     i;
	uint8_t st;

	log("[ide][id] slave=%d: waiting !BSY (max 1000 polls)", dev->slave);

	for (i = 0; i < 1000; i++) {
		st = ide_read_reg(channel, ATA_REG_STATUS);
		if (!(st & ATA_SR_BSY))
			break;
	}

	if (st & ATA_SR_BSY) {
		log("[ide][id] slave=%d: TIMEOUT waiting for !BSY (last status=0x%02x)", dev->slave, st);
		return -1;
	}

	log("[ide][id] slave=%d: !BSY reached, status=0x%02x", dev->slave, st);

	ide_write_reg(channel, ATA_REG_HDDEVSEL, (uint8_t)(0xA0 | (dev->slave << 4)));
	ide_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	log("[ide][id] slave=%d: issued IDENTIFY (HDDEVSEL=0x%02x)", dev->slave,
	    (uint8_t)(0xA0 | (dev->slave << 4)));

	for (i = 0; i < 10000; i++) {
		st = ide_read_reg(channel, ATA_REG_STATUS);
		if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
			break;
		if (st & ATA_SR_ERR) {
			log("[ide][id] slave=%d: ERR flag set (status=0x%02x)", dev->slave, st);
			return -1;
		}
	}

	if (!(st & ATA_SR_DRQ)) {
		log("[ide][id] slave=%d: no DRQ (status=0x%02x, i=%d)", dev->slave, st, i);
		return -1;
	}
	log("[ide][id] slave=%d: DRQ ready, reading 128 dwords (status=0x%02x, i=%d)", dev->slave, st,
	    i);

	insl(channel->base_port + ATA_REG_DATA, buf, 128);
	st = ide_read_reg(channel, ATA_REG_STATUS);
	if (st & ATA_SR_ERR) {
		log("[ide][id] slave=%d: ERR after IDENTIFY transfer (status=0x%02x)", dev->slave, st);
		return -1;
	}
	log("[ide][id] slave=%d: IDENTIFY complete, post-status=0x%02x", dev->slave, st);

	return 0;
}

static int ide_populate_drive(struct ide_channel *channel, struct ide_device *dev)
{
	uint16_t id_buf[256];

	log("[ide][pop] slave=%d: base_port=0x%04x", dev->slave, channel->base_port);

	if (ide_identify(channel, dev, id_buf) != 0) {
		log("[ide][pop] slave=%d: ide_identify FAILED", dev->slave);
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

	log("[ide][pop] slave=%d: OK  model=\"%s\" serial=\"%s\" type=%u cyl=%u head=%u sect=%u lba=%u "
	    "cap=0x%04x fvalid=0x%02x",
	    dev->slave, dev->model, dev->serial, dev->dev_type, dev->cylinders, dev->heads,
	    dev->sectors, dev->lba_max_sectors, dev->capabilities, dev->field_valid);

	return 0;
}

static void ide_probe_channels(void)
{
	struct ide_channel *channel;
	struct ide_device  *dev;
	int                 ch_count = 0, dev_count = 0, present_count = 0;

	list_for_each_entry(channel, &ide_channels, node)
	{
		int slave;

		log("[ide][chan] probing channel base=0x%04x", channel->base_port);
		ch_count++;

		for (slave = 0; slave < 2; slave++) {
			dev = kmalloc(sizeof(struct ide_device), GFP_KERNEL);
			if (!dev) {
				log("[ide][chan] channel base=0x%04x slave=%d: kmalloc FAILED", channel->base_port,
				    slave);
				continue;
			}
			dev->channel = channel;
			dev->slave   = slave;
			dev->present = (ide_populate_drive(channel, dev) == 0);
			dev_count++;
			if (dev->present) {
				present_count++;
				list_add_tail(&dev->node, &ide_devices);
			} else {
				log("[ide][chan] channel base=0x%x slave=%d: NOT PRESENT", channel->base_port,
				    slave);
				kfree(dev);
			}
		}
	}

	log("[ide][chan] probed %d channels, %d drives attempted, %d found", ch_count, dev_count,
	    present_count);
}

void ide_init(void)
{
	log(" -= Starting IDE drives probing =-");

	pci_for_each_device(PCI_DEVICE_IDE_CONTROLLER, ide_probe_controller, NULL);
	ide_probe_channels();

	struct ide_device *dev;
	int                n = 0;
	list_for_each_entry(dev, &ide_devices, node)
	{
		log("ATA[%d] base=0x%x slave=%d: model=\"%s\" serial=\"%s\" type=%u cyl=%u head=%u sect=%u "
		    "lba=%u size=%uMiB",
		    n++, dev->channel->base_port, dev->slave, dev->model, dev->serial, dev->dev_type,
		    dev->cylinders, dev->heads, dev->sectors, dev->lba_max_sectors,
		    dev->lba_max_sectors / 2048 /* assume 512B sectors */);
	}

	if (n == 0) {
		log("NO IDE DEVICES FOUND");
	}

	log(" -= IDE drives probing finished =-");
}
