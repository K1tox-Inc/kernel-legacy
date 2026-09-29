#include <fs/ext2.h>
#include <kernel/block.h>

int ext2_parse_superblock(struct block_device *dev)
{
	char                           buffer[EXT2_INITIAL_BLOCK_SIZE];
	const struct ext2_super_block *sb = (struct ext2_super_block *)buffer;

	blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE);
	blkdev_read(dev, 1, 1, (void *)buffer);

	if (sb->s_magic != EXT2_MAGIC) {
		vga_printf("ext2parse: not an ext2 filesystem (invalid magic)\n");
		return -1;
	}

	blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE << sb->s_log_block_size);

	vga_printf("Device: %s\n", dev->name);
	vga_printf("  block_size:   %u bytes\n", dev->block_size);
	vga_printf("  sector_size:  %u bytes\n", dev->sector_size);
	vga_printf("  inodes:       %u\n", sb->s_inodes_count);
	vga_printf("  blocks:       %u\n", sb->s_blocks_count);
	vga_printf("  inodes/group: %u\n", sb->s_inodes_per_group);
	vga_printf("  blocks/group: %u\n", sb->s_blocks_per_group);
	vga_printf("  inode_size:   %u bytes\n", sb->s_inode_size);

	return 0;
}
