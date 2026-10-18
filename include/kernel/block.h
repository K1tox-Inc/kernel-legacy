#pragma once

#include <list.h>

#define DISK_NAME_LEN 16

struct generic_disk;

struct disk_ops {
	int (*read_sectors)(struct generic_disk *d, size_t lba, size_t count, void *buf);
	int (*write_sectors)(struct generic_disk *d, size_t lba, size_t count, const void *buf);
	int (*flush)(struct generic_disk *d);
};

struct generic_disk {
	char                   name[DISK_NAME_LEN]; // "hda"
	size_t                 nr_sectors;
	const struct disk_ops *ops;
	void                  *priv; // ata_device *
};

struct block_device {                         // a partition, or the whole disk
	char                 name[DISK_NAME_LEN]; // "hda1"
	struct generic_disk *disk;
	struct block_device *parent; // non-null only for partitions
	size_t               start_lba, nr_sectors;
	struct list_head     node;
};

extern int add_disk(struct generic_disk *d);
