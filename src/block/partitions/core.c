#include <kernel/block.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>
#include <utils/error.h>

#include "msdos.h"

LIST_HEAD(block_devices);

static int (*const partition_parsers[])(const struct block_device *, struct disk_partition *,
                                        size_t max) = {mbr_parse};

static struct block_device *blkdev_create(struct generic_disk *disk, const char *name,
                                          size_t lba_start, size_t nr_sectors,
                                          struct block_device *parent)
{
	struct block_device *dev = kmalloc(sizeof(struct block_device), __GFP_KERNEL | __GFP_ZERO);

	if (dev == NULL) {
		return NULL;
	}

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
	if (bsize == dev->block_size)
		return 0;

	if (!bsize || bsize % dev->sector_size)
		return -EINVAL;

	*((size_t *)&dev->block_size) = bsize;

	return 0;
}

int blkdev_register_disk(struct generic_disk *disk)
{
	struct block_device *dev = blkdev_create(disk, disk->name, 0, disk->nr_sectors, NULL);

	if (dev == NULL) {
		log("Failed to allocate `struct block_device', skipping...");
		return -1;
	}

	list_add_tail(&dev->node, &block_devices);

	char   name[DISK_NAME_LEN];
	size_t name_len = ft_strlen(dev->name); // bug-prone: name is non-null terminated

	int                   nr_parts = 0;
	struct disk_partition parts[16];
	for (uint32_t i = 0; i < ARRAY_SIZE(partition_parsers); i++) {
		nr_parts = partition_parsers[i](dev, parts, 16);

		if (nr_parts < 0) {
			log("Error");
		} else if (nr_parts)
			break;
	}

	for (int i = 0; i < nr_parts; i++) {
		ft_memcpy(name, disk->name, DISK_NAME_LEN);
		name[name_len]     = 'p';
		name[name_len + 1] = ((char[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
		                               'c', 'd', 'e', 'f'})[i];
		name[name_len + 2] = 0;

		struct block_device *part =
		    blkdev_create(disk, name, parts[i].lba_start, parts[i].nr_sectors, dev);

		if (dev == NULL) {
			log("Failed to allocate `struct block_device', skipping...");
			continue;
		}

		list_add_tail(&part->node, &block_devices);
	}

	return 0;
}
