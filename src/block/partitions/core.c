#include <kernel/block.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>
#include <uapi/errno.h>
#include <utils/assert.h>

#include "msdos.h"

LIST_HEAD(block_devices);

static int (*const partition_parsers[])(const struct block_device *, struct disk_partition *,
                                        size_t max) = {mbr_parse};

static struct block_device *blkdev_create(struct generic_disk *disk, const char *name,
                                          size_t lba_start, size_t nr_sectors,
                                          struct block_device *__nullable parent)
{
	assert(disk && name);

	struct block_device *dev = kmalloc(sizeof(struct block_device), __GFP_KERNEL | __GFP_ZERO);

	if (dev == NULL)
		return NULL;

	dev->disk   = disk;
	dev->parent = parent;

	ft_memcpy(dev->name, name, DISK_NAME_LEN);

	dev->nr_sectors = nr_sectors;
	dev->lba_start  = lba_start;

	*((size_t *)&dev->sector_size) = 512; // hardcoded default
	*((size_t *)&dev->block_size)  = dev->sector_size;

	return dev;
}

int blkdev_set_block_size(struct block_device *dev, size_t bsize)
{
	assert(dev);

	if (bsize == dev->block_size)
		return 0;

	if (!bsize || bsize % dev->sector_size)
		return -EINVAL;

	*((size_t *)&dev->block_size) = bsize;

	return 0;
}

int blkdev_register_disk(struct generic_disk *disk)
{
	assert(disk);

	struct block_device *dev = blkdev_create(disk, disk->name, 0, disk->nr_sectors, NULL);

	if (dev == NULL)
		return -ENOMEM;

	int                   nr_parts = 0;
	struct disk_partition parts[16];
	for (uint32_t i = 0; i < ARRAY_SIZE(partition_parsers); i++) {
		nr_parts = partition_parsers[i](dev, parts, ARRAY_SIZE(parts));

		if (nr_parts != 0)
			break;
	}

	if (nr_parts < 0) {
		kfree(dev);
		return nr_parts;
	}

	list_add_tail(&dev->node, &block_devices);

	size_t name_len = 0;
	while (name_len < DISK_NAME_LEN && disk->name[name_len])
		name_len++;

	if (name_len + 3 > DISK_NAME_LEN)
		return 0;

	char name[DISK_NAME_LEN] = {0};
	ft_memcpy(name, disk->name, name_len);
	name[name_len] = 'p';

	for (int i = 0; i < nr_parts; i++) {
		name[name_len + 1] = "0123456789abcdef"[i];

		struct block_device *part =
		    blkdev_create(disk, name, parts[i].lba_start, parts[i].nr_sectors, dev);

		if (part == NULL)
			continue;

		list_add_tail(&part->node, &block_devices);
	}

	return 0;
}
