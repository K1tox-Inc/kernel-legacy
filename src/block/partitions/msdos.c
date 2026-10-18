#include <kernel/block.h>
#include <libk.h>

#include "msdos.h"

int mbr_parse(const struct block_device *dev, struct disk_partition *parts, size_t max)
{
	uint8_t                 sector[MBR_SECTOR_SIZE];
	const struct mbr_table *table = (struct mbr_table *)(sector + MBR_PARTITION_GAP);

	if (dev->disk->ops->read_sectors(dev->disk, 0, 1, sector) < 0) {
		log("Unable to read boot sector, skipping...");
		return -1;
	}

	if (table->signature != MBR_SIGNATURE)
		return 0;

	int found_partitions = 0;
	for (size_t i = 0; i < MIN(max, MBR_MAX_PARTITIONS); i++) {
		if (!table->entries[i].partition_type)
			continue;

		parts[i].lba_start  = table->entries[i].lba_start;
		parts[i].nr_sectors = table->entries[i].nr_sectors;

		found_partitions++;
	}

	return found_partitions;
}
