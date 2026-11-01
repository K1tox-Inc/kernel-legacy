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

/* Converts `count` blocks at `lba` to disk sectors, or fails if they don't fit in `dev` */
static __always_inline int blkdev_to_sectors(const struct block_device *dev, size_t lba,
                                             size_t count, size_t *start, size_t *nr_sectors)
{
	const size_t sector_per_block = dev->block_size / dev->sector_size;
	const size_t nr_blocks        = dev->nr_sectors / sector_per_block;

	if (count > nr_blocks || lba > nr_blocks - count)
		return -EIO;

	*start      = dev->lba_start + lba * sector_per_block;
	*nr_sectors = count * sector_per_block;

	return 0;
}

static __always_inline int blkdev_read(const struct block_device *dev, size_t lba, size_t count,
                                       void *buf)
{
	size_t start, nr_sectors;

	if (blkdev_to_sectors(dev, lba, count, &start, &nr_sectors) < 0)
		return -EIO;

	return dev->disk->ops->read(dev->disk, start, nr_sectors, buf) ? -EIO : 0;
}

static __always_inline int blkdev_write(const struct block_device *dev, size_t lba, size_t count,
                                        const void *buf)
{
	size_t start, nr_sectors;

	if (blkdev_to_sectors(dev, lba, count, &start, &nr_sectors) < 0)
		return -EIO;

	return dev->disk->ops->write(dev->disk, start, nr_sectors, buf) ? -EIO : 0;
}

extern int blkdev_register_disk(struct generic_disk *d);
extern int blkdev_set_block_size(struct block_device *dev, size_t bsize);
