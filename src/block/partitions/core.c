#include <kernel/block.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>

#include "msdos.h"

LIST_HEAD(block_devices);

static int (*const partition_parsers[])(const struct block_device *, struct disk_partition *,
                                        size_t max) = {mbr_parse};

int add_disk(struct generic_disk *disk)
{
	struct block_device *dev = kmalloc(sizeof(struct block_device), __GFP_KERNEL | __GFP_ZERO);

	if (dev == NULL) {
		log("Failed to allocate `struct block_device', skipping...");
		return -1;
	}

	dev->disk = disk;

	ft_memcpy(dev->name, disk->name, DISK_NAME_LEN);
	size_t name_len = ft_strlen(dev->name); // bug-prone: name is non-null terminated

	dev->nr_sectors = disk->nr_sectors;
	dev->lba_start  = 0;

	list_add_tail(&dev->node, &block_devices);

	int                   nr_parts = 0;
	struct disk_partition parts[16];
	for (uint32_t i = 0; i < (sizeof(partition_parsers) / sizeof(*partition_parsers)); i++) {
		nr_parts = partition_parsers[i](dev, parts, 16);

		if (nr_parts < 0)
			log("Error");
		if (nr_parts)
			break;
	}

	for (int i = 0; i < nr_parts; i++) {
		struct block_device *part = kmalloc(sizeof(struct block_device), GFP_KERNEL);

		if (dev == NULL) {
			log("Failed to allocate `struct block_device', skipping...");
			continue;
		}

		part->disk = disk;

		ft_memcpy(part->name, disk->name, DISK_NAME_LEN);
		part->name[name_len]     = 'p';
		part->name[name_len + 1] = ((char[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a',
		                                     'b', 'c', 'd', 'e', 'f'})[i];
		part->name[name_len + 2] = 0;

		part->nr_sectors = parts[i].nr_sectors;
		part->lba_start  = parts[i].lba_start;

		part->parent = dev;

		list_add_tail(&part->node, &block_devices);
	}

	return 0;
}
