#pragma once

#include <kernel/block.h>
#include <types.h>
#include <utils/compiler.h>

#define MBR_SIGNATURE      0xAA55
#define MBR_SECTOR_SIZE    512
#define MBR_PARTITION_GAP  0x1B8
#define MBR_PARTITION_SIZE 16
#define MBR_MAX_PARTITIONS 4

#define MBR_TYPE_FAT12      0x01
#define MBR_TYPE_FAT16_OLD  0x04
#define MBR_TYPE_FAT16      0x06
#define MBR_TYPE_EXTENDED   0x05
#define MBR_TYPE_HPFS_NTFS  0x07
#define MBR_TYPE_FAT32      0x0B
#define MBR_TYPE_FAT32_LBA  0x0C
#define MBR_TYPE_LINUX_SWAP 0x82
#define MBR_TYPE_LINUX      0x83
#define MBR_TYPE_LINUX_LVM  0x8E
#define MBR_TYPE_FREEBSD    0x81
#define MBR_TYPE_OPENBSD    0x81
#define MBR_TYPE_NETBSD     0x82
#define MBR_TYPE_SOLARIS    0x82
#define MBR_TYPE_PROTECTED  0xEE

struct mbr_partition {
	uint8_t boot_indicator : 1;
	uint8_t : 7;
	uint32_t chs_start_addr : 24;
	uint8_t  partition_type;
	uint32_t chs_last_part_addr : 24;
	uint32_t lba_start;
	uint32_t nr_sectors;
} __packed;

struct mbr_table {
	uint8_t opt_unique_id[4]; // optional
	uint16_t : 16;
	struct mbr_partition entries[MBR_MAX_PARTITIONS];
	uint16_t             signature;
} __packed;

extern int mbr_parse(const struct block_device *dev, struct disk_partition *parts, size_t max);
