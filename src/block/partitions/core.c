#include <kernel/block.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>

LIST_HEAD(block_devices);

int add_disk(struct generic_disk *disk)
{
	struct block_device *dev = kmalloc(sizeof(struct block_device), __GFP_KERNEL | __GFP_ZERO);

	if (dev == NULL) {
		log("Failed to allocate `struct block_device', skipping...");
		return -1;
	}

	dev->disk = disk;
	ft_memcpy(dev->name, disk->name, DISK_NAME_LEN);

	list_add_tail(&dev->node, &block_devices);

	return 0;
}
