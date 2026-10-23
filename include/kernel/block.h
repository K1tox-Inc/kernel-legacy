#pragma once

#include <list.h>
#include <uapi/errno.h>
#include <utils/compiler.h>

#define DISK_NAME_LEN 16

struct generic_disk;

struct disk_ops {
	int (*read)(struct generic_disk *d, size_t lba, size_t count, void *buf);
	int (*write)(struct generic_disk *d, size_t lba, size_t count, const void *buf);
	int (*flush)(struct generic_disk *d);
};

struct generic_disk {
	char                   name[DISK_NAME_LEN];
	size_t                 nr_sectors;
	const struct disk_ops *ops;
	void                  *priv;
};

struct disk_partition {
	size_t lba_start;
	size_t nr_sectors;
};

struct block_device {
	char                 name[DISK_NAME_LEN];
	struct generic_disk *disk;
	struct block_device *parent; // non-null only for partitions
	size_t               lba_start, nr_sectors;
	const size_t         sector_size, block_size;
	struct list_head     node;
};

static __always_inline int blkdev_read(const struct block_device *dev, size_t lba, size_t count,
                                       void *buf)
{
	size_t sector_per_block = dev->block_size >> 9;
	size_t nr_sectors       = count * sector_per_block;
	size_t relative_lba     = dev->lba_start + lba * sector_per_block;

	return dev->disk->ops->read(dev->disk, relative_lba, nr_sectors, buf) ? -EIO : 0;
}

static __always_inline int blkdev_write(const struct block_device *dev, size_t lba, size_t count,
                                        const void *buf)
{
	size_t sector_per_block = dev->block_size >> 9;
	size_t nr_sectors       = count * sector_per_block;
	size_t relative_lba     = dev->lba_start + lba * sector_per_block;

	return dev->disk->ops->write(dev->disk, relative_lba, nr_sectors, buf) ? -EIO : 0;
}

extern int blkdev_register_disk(struct generic_disk *d);
extern int blkdev_set_block_size(struct block_device *dev, size_t bsize);
