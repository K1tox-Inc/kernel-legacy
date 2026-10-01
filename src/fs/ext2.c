#include <bitmap/bitmap.h>
#include <fs/ext2.h>
#include <kernel/block.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <utils/error.h>
#include <utils/kmacro.h>
#include <vga/vga.h>

#define get_dentry_name(dentry)                                                                    \
	((char *)&(dentry)->name - !!(sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE))

static const struct block_device     *_dev       = NULL;
static const struct ext2_super_block *sb         = NULL;
static const struct ext2_inode       *root_inode = NULL;

void ext2_print_group_descriptor(const struct ext2_group_desc *gd)
{
	vga_printf("\n");
	vga_printf("Block Descriptor:\n");
	vga_printf("  Block address of block usage bitmap   %u\n", gd->bg_block_bitmap);
	vga_printf("  Block address of inode usage bitmap   %u\n", gd->bg_inode_bitmap);
	vga_printf("  Starting block address of inode table %u\n", gd->bg_inode_table);
	vga_printf("  Number of unallocated blocks in group %u\n", gd->bg_free_blocks_count);
	vga_printf("  Number of unallocated inodes in group %u\n", gd->bg_free_inodes_count);
	vga_printf("  Number of directories in group        %u\n", gd->bg_used_dirs_count);
}

void ext2_print_inode(const struct ext2_inode *ino)
{
	const uint16_t mode  = ino->i_mode;
	const uint16_t type  = mode >> 12;
	const uint16_t perms = mode & 0x0FFF;

	const char *type_name;
	switch (type) {
	case 0x1:
		type_name = "FIFO";
		break;
	case 0x2:
		type_name = "CharDev";
		break;
	case 0x4:
		type_name = "Dir";
		break;
	case 0x6:
		type_name = "BlkDev";
		break;
	case 0x8:
		type_name = "RegFile";
		break;
	case 0xA:
		type_name = "SymLink";
		break;
	case 0xC:
		type_name = "Socket";
		break;
	default:
		type_name = "unknown";
	}

	char perm_str[11];
	perm_str[0] = (perms >> 8) & 0x4 ? 'r' : '-';
	perm_str[1] = (perms >> 8) & 0x2 ? 'w' : '-';
	perm_str[2] = (perms >> 8) & 0x1 ? 'x' : '-';
	perm_str[3] = (perms >> 5) & 0x4 ? 'r' : '-';
	perm_str[4] = (perms >> 5) & 0x2 ? 'w' : '-';
	perm_str[5] = (perms >> 5) & 0x1 ? 'x' : '-';
	perm_str[6] = (perms >> 2) & 0x4 ? 'r' : '-';
	perm_str[7] = (perms >> 2) & 0x2 ? 'w' : '-';
	perm_str[8] = (perms >> 2) & 0x1 ? 'x' : '-';
	perm_str[9] = '\0';

	vga_printf("\n");
	vga_printf("Inode:\n");
	vga_printf("  Type:    %s (0x%x)\n", type_name, type);
	vga_printf("  Perms:   %s (0x%x)\n", perm_str, perms);
	vga_printf("  UID:     %u\n", ino->i_uid);
	vga_printf("  GID:     %u\n", ino->i_gid);
	vga_printf("  Size:    %u bytes (%u KiB)\n", ino->i_size, ino->i_size / 1024);
	vga_printf("  Links:   %u\n", ino->i_links_count);
	vga_printf("  Sectors: %u (%u KiB on disk)\n", ino->i_nr_sectors,
	           ino->i_nr_sectors * _dev->sector_size / 1024);
	vga_printf("  Flags:   0x%x\n", ino->i_flags);
	vga_printf("  atime:   %u\n", ino->i_atime);
	vga_printf("  mtime:   %u\n", ino->i_mtime);
	vga_printf("  ctime:   %u\n", ino->i_ctime);
	vga_printf("  dtime:   %u\n", ino->i_dtime);

	vga_printf("  Blocks:  [");
	for (int b = 0; b < 15; b++) {
		if (b > 0)
			vga_printf(", ");
		vga_printf("%u", ino->i_block[b]);
	}
	vga_printf("]\n");

	vga_printf("    Direct (0-11):  ");
	for (int b = 0; b < 12; b++) {
		if (b > 0)
			vga_printf(", ");
		vga_printf("%u", ino->i_block[b]);
	}
	vga_printf("\n");

	vga_printf("    Single indirect (12):  %u\n", ino->i_block[12]);
	vga_printf("    Double indirect (13):  %u\n", ino->i_block[13]);
	vga_printf("    Triple indirect (14):  %u\n", ino->i_block[14]);
}

int ext2_cat(const struct ext2_inode *ino)
{
	if (EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir))
		return -EISDIR;

	else if (!EXT2_InodeTypeCheck(*ino, Ext2_Inode_RegFile)) {
		vga_printf("cat: Not handled inode type (%u)\n", ino->i_mode >> 12);
		return -1;
	}

	char *buffer = kmalloc(MAX(ino->i_size, _dev->block_size), GFP_KERNEL);

	for (int j = 0; ino->i_block[j] && j < 12; j++) {
		blkdev_read(_dev, ino->i_block[0], 1, buffer);

		for (size_t i = 0; i < MIN(ino->i_size, _dev->block_size); i++)
			vga_printf("%c", buffer[i]);
	}

	kfree(buffer);

	return 0;
}

int ext2_list_dentries(const struct ext2_inode *ino)
{
	if (!EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir))
		return -ENOTDIR;

	for (int j = 0; ino->i_block[j] && j < 12; j++) {
		const struct ext2_dir_entry *const dentries = kmalloc(_dev->block_size, GFP_KERNEL),
		                                   *dentry  = dentries;

		blkdev_read(_dev, ino->i_block[j], 1, (void *)dentries);

		for (; (uintptr_t)dentry - (uintptr_t)dentries < ino->i_size;
		     dentry = (void *)((uintptr_t)dentry + dentry->rec_len)) {
			if (dentry->inode == 0)
				continue;

			char *name = kmalloc(dentry->name_len + 1, GFP_KERNEL);

			ft_memcpy(name, get_dentry_name(dentry), dentry->name_len);

			name[dentry->name_len] = 0;

			vga_printf("'%s'\n", name);

			kfree(name);
		}

		kfree((void *)dentries);
	}

	return 0;
}

int ext2_get_dentries(const struct ext2_inode *ino, struct ext2_dir_entry *m)
{
	if (!(EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir)))
		return -ENOTDIR;

	for (int j = 0; ino->i_block[j] && j < 12; j++) {
		blkdev_read(_dev, ino->i_block[j], 1, (uint8_t *)m + _dev->block_size * j);
	}

	int nr_dentries = 0;
	for (const struct ext2_dir_entry *dentry = m; (uintptr_t)dentry - (uintptr_t)m < ino->i_size;
	     dentry = (void *)((uint8_t *)dentry + dentry->rec_len), nr_dentries++)
		;

	return nr_dentries;
}

static int ext2_resolve_dentry(const struct ext2_dir_entry *dentry, struct ext2_inode *ino)
{
	uint8_t *block = kmalloc(_dev->block_size, GFP_KERNEL);

	size_t group_indx = (dentry->inode - 1) / sb->s_inodes_per_group;
	size_t inode_indx = (dentry->inode - 1) % sb->s_inodes_per_group;

	size_t group_dscr_per_block = (_dev->block_size / sizeof(struct ext2_group_desc));
	size_t group_start          = group_indx / group_dscr_per_block + 1 + sb->s_first_data_block;

	size_t inode_per_block = _dev->block_size / sb->s_inode_size;

	dbg(group_indx);
	dbg(inode_indx);

	dbg(group_dscr_per_block);
	dbg(group_start);

	dbg(inode_per_block);

	const struct ext2_group_desc *const gd =
	    (struct ext2_group_desc *)block + group_indx % group_dscr_per_block;

	blkdev_read(_dev, group_start, 1, (void *)block);

	ext2_print_group_descriptor(gd);

	blkdev_read(_dev, gd->bg_inode_table + (inode_indx / inode_per_block), 1, (void *)block);

	ft_memcpy(ino, (struct ext2_inode *)block + inode_indx % inode_per_block,
	          sizeof(struct ext2_inode));

	kfree(block);

	return 0;
}

int ext2_lookup(const char *path, struct ext2_inode *ino)
{
	if (!_dev || !sb || !root_inode) {
		vga_printf("%s: Did you run `ext2parse`?\n", __func__);
		return -EAGAIN;
	}

	if (*path != '/') {
		vga_printf("%s: Not handle relative paths.\n", __func__);
		return -ENOENT;
	}

	ft_memcpy(ino, root_inode, sizeof(struct ext2_inode));

	if (*path == '/')
		path++;

	for (const char *lkup = path; *lkup;) {
		int next_slash = 0;
		while (lkup[next_slash] && lkup[next_slash] != '/')
			next_slash++;

		struct ext2_dir_entry *dentries = kmalloc(ino->i_size, GFP_KERNEL);
		int                    ret      = ext2_get_dentries(ino, dentries);

		if (ret < 0)
			return ret;

		const struct ext2_dir_entry *dentry = dentries;
		for (; (uintptr_t)dentry - (uintptr_t)dentries < ino->i_size;
		     dentry = (void *)((uint8_t *)dentry + dentry->rec_len)) {
			if (dentry->inode == 0)
				continue;
			if (ft_memcmp(lkup, get_dentry_name(dentry), next_slash) != 0)
				continue;

			ext2_resolve_dentry(dentry, ino);

			break;
		}

		lkup += next_slash + (lkup[next_slash] == '/');

		if ((uintptr_t)dentry >= (uintptr_t)dentries + _dev->block_size)
			return -ENOENT;

		kfree(dentries);
	}

	return 0;
}

int ext2_parse_superblock(struct block_device *dev)
{
	_dev = dev;
	sb   = kmalloc(sizeof(struct ext2_super_block *), GFP_KERNEL);

	blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE);
	blkdev_read(dev, 1, 1, (void *)sb);

	if (sb->s_magic != EXT2_MAGIC) {
		vga_printf("ext2parse: Not an ext2 filesystem (invalid magic)\n");
		return -1;
	}

	blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE << sb->s_log_block_size);

	vga_printf("Device: %s\n", dev->name);
	vga_printf("  Ext2 version  %u.%u\n", sb->s_rev_level, sb->s_minor_rev_level);
	vga_printf("  block_size:   %u bytes\n", dev->block_size);
	vga_printf("  sector_size:  %u bytes\n", dev->sector_size);
	vga_printf("  inodes:       %u\n", sb->s_inodes_count);
	vga_printf("  blocks:       %u\n", sb->s_blocks_count);
	vga_printf("  inodes/group: %u\n", sb->s_inodes_per_group);
	vga_printf("  blocks/group: %u\n", sb->s_blocks_per_group);
	vga_printf("  inode_size:   %u bytes\n", sb->s_inode_size);

	if (((sb->s_rev_level << 8) | sb->s_minor_rev_level) != 0x0100) {
		vga_printf("ext2parse: Not handled Ext2 Rev. (%u.%u), aborting...\n", sb->s_rev_level,
		           sb->s_minor_rev_level);
		return -1;
	}

	if (sb->s_inode_size != sizeof(struct ext2_inode)) {
		vga_printf("ext2parse: Not handled inode size (%u), aborting...\n", sb->s_inode_size);
		return -1;
	}

	const struct ext2_group_desc *const gd = kmalloc(dev->block_size, GFP_KERNEL);
	blkdev_read(dev, (dev->block_size == EXT2_INITIAL_BLOCK_SIZE) + 1, 1, (void *)gd);

	const struct ext2_inode *const inodes = (void *)gd;
	blkdev_read(dev, gd->bg_inode_table, 1, (void *)inodes);
	root_inode = kmalloc(sizeof(struct ext2_inode), GFP_KERNEL);
	ft_memcpy((void *)root_inode, inodes + 1, sizeof(struct ext2_inode));

	kfree((void *)gd);

	return 0;
}
