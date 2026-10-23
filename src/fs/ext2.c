#include <bitmap/bitmap.h>
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <kernel/bcache.h>
#include <kernel/block.h>
#include <kernel/io_stream.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>
#include <uapi/errno.h>
#include <utils/assert.h>
#include <utils/compiler.h>
#include <utils/kmacro.h>
#include <vga/vga.h>

static __always_inline size_t ext2_dir_buf_size(const struct block_device *dev,
                                                const struct ext2_inode   *ino)
{
	assert(dev && ino);

	const size_t bsize = dev->block_size;

	return DIV_ROUND_UP(ino->i_size, bsize) * bsize;
}

static int ext2_inode_bmap(struct block_device *dev, const struct ext2_inode *ino, size_t blk_idx,
                           uint32_t *out)
{
	assert(dev && ino && out);

	*out = 0;

	if (blk_idx < 12) {
		*out = ino->i_block[blk_idx];
		return 0;
	}

	const size_t ptrs_per_blk = dev->block_size / sizeof(uint32_t);

	uint32_t blk;
	size_t   stride;

	blk_idx -= 12;

	if (blk_idx < ptrs_per_blk) {
		blk    = ino->i_block[12];
		stride = 1;
	} else {
		blk_idx -= ptrs_per_blk;
		if (blk_idx / ptrs_per_blk < ptrs_per_blk) {
			blk    = ino->i_block[13];
			stride = ptrs_per_blk;
		} else {
			blk_idx -= ptrs_per_blk * ptrs_per_blk;
			if (blk_idx / ptrs_per_blk / ptrs_per_blk < ptrs_per_blk) {
				blk    = ino->i_block[14];
				stride = ptrs_per_blk * ptrs_per_blk;
			} else {
				return -EFBIG;
			}
		}
	}

	if (!blk)
		return 0;

	for (; blk && stride; stride /= ptrs_per_blk) {
		struct bcache *b = bread(dev, blk);
		if (!b)
			return -EIO;

		blk = ((uint32_t *)b->data)[(blk_idx / stride) % ptrs_per_blk];
		brelse(b);
	}

	*out = blk;

	return 0;
}

static __always_inline bool ext2_dentry_valid(const struct ext2_dentry *dentry, size_t remaining)
{
	const size_t hdr_len = sizeof(struct ext2_dentry);

	return remaining >= hdr_len && dentry->rec_len >= hdr_len && dentry->rec_len <= remaining &&
	       hdr_len + dentry->name_len <= dentry->rec_len;
}

int ext2_get_dentries(struct block_device *dev, const struct ext2_inode *ino, struct ext2_dentry *m)
{
	assert(dev && ino && m);

	if (!(EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir)))
		return -ENOTDIR;

	size_t nr_blocks = DIV_ROUND_UP(ino->i_size, dev->block_size);

	for (size_t j = 0; j < nr_blocks; j++) {
		uint32_t blk;
		int      ret = ext2_inode_bmap(dev, ino, j, &blk);

		if (ret == 0 && blk)
			ret = blkdev_read(dev, blk, 1, (uint8_t *)m + dev->block_size * j);
		else if (ret == 0)
			ft_memset((uint8_t *)m + dev->block_size * j, 0, dev->block_size);

		if (ret < 0)
			return ret;
	}

	int nr_dentries = 0;
	for (size_t off = 0; off < ino->i_size; nr_dentries++) {
		const struct ext2_dentry *dentry = (void *)((uint8_t *)m + off);

		if (!ext2_dentry_valid(dentry, ino->i_size - off))
			return -EIO;

		off += dentry->rec_len;
	}

	return nr_dentries;
}

static ssize_t ext2_inode_read(struct block_device *dev, const struct ext2_inode *ino, size_t off,
                               void *buf, size_t len)
{
	assert(dev && ino && buf);

	if (off >= ino->i_size)
		return 0;

	len = MIN(len, ino->i_size - off);

	uint8_t *block = kmalloc(dev->block_size, GFP_KERNEL);
	size_t   done  = 0;
	int      ret   = 0;

	if (unlikely(!block))
		return -ENOMEM;

	while (done < len) {
		size_t   pos     = off + done;
		size_t   blk_idx = pos / dev->block_size;
		size_t   blk_off = pos % dev->block_size;
		size_t   chunk   = MIN(len - done, dev->block_size - blk_off);
		uint32_t blk;

		ret = ext2_inode_bmap(dev, ino, blk_idx, &blk);
		if (ret == 0 && blk)
			ret = blkdev_read(dev, blk, 1, block);
		if (ret < 0)
			break;

		if (blk)
			ft_memcpy((uint8_t *)buf + done, block + blk_off, chunk);
		else
			ft_memset((uint8_t *)buf + done, 0, chunk);

		done += chunk;
	}

	kfree(block);

	return ret < 0 ? ret : (ssize_t)done;
}

static int ext2_read_inode(const struct block_device *dev, const struct ext2_super_block *sb,
                           uint32_t ino_nr, struct ext2_inode *ino)
{
	assert(dev && sb && ino);

	if (ino_nr == 0 || ino_nr > sb->s_inodes_count)
		return -EIO;

	uint8_t *block = kmalloc(dev->block_size, GFP_KERNEL);
	if (unlikely(!block))
		return -ENOMEM;

	size_t group_indx = (ino_nr - 1) / sb->s_inodes_per_group;
	size_t inode_indx = (ino_nr - 1) % sb->s_inodes_per_group;

	size_t group_dscr_per_block = (dev->block_size / sizeof(struct ext2_group_desc));
	size_t group_start          = group_indx / group_dscr_per_block + 1 + sb->s_first_data_block;

	size_t inode_per_block = dev->block_size / sb->s_inode_size;

	const struct ext2_group_desc *const gd =
	    (struct ext2_group_desc *)block + group_indx % group_dscr_per_block;

	int ret = blkdev_read(dev, group_start, 1, (void *)block);
	if (ret < 0)
		goto end;

	ret = blkdev_read(dev, gd->bg_inode_table + (inode_indx / inode_per_block), 1, (void *)block);
	if (ret < 0)
		goto end;

	ft_memcpy(ino, (struct ext2_inode *)block + inode_indx % inode_per_block,
	          sizeof(struct ext2_inode));

end:
	kfree(block);

	return ret;
}

static int ext2_find_inode(const struct superblock *sb, const struct ext2_inode *dir,
                           const char *name, size_t name_len, struct ext2_inode *out)
{
	assert(sb && dir && name && out);

	if (!EXT2_InodeTypeCheck(*dir, Ext2_Inode_Dir))
		return -ENOTDIR;

	if (name_len > EXT2_NAME_LEN)
		return -ENAMETOOLONG;

	const size_t        dir_size = dir->i_size;
	struct ext2_dentry *dentries = kmalloc(ext2_dir_buf_size(sb->s_bdev, dir), GFP_KERNEL);

	if (unlikely(!dentries))
		return -ENOMEM;

	int ret = ext2_get_dentries(sb->s_bdev, dir, dentries);
	if (ret < 0)
		goto end;

	ret = -ENOENT;
	for (const struct ext2_dentry *dentry = dentries;
	     (uintptr_t)dentry - (uintptr_t)dentries < dir_size;
	     dentry = (void *)((uint8_t *)dentry + dentry->rec_len)) {
		if (dentry->inode == 0 || dentry->name_len != name_len)
			continue;
		if (ft_memcmp(name, dentry->name, name_len) != 0)
			continue;

		ret = ext2_read_inode(sb->s_bdev, sb->s_fs_info, dentry->inode, out);
		break;
	}

end:
	kfree(dentries);

	return ret;
}

static int ext2_f_open(struct file *f)
{
	const struct ext2_inode *const ino = f->f_inode->i_prv;

	if (EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir))
		return -EISDIR;

	if (!EXT2_InodeTypeCheck(*ino, Ext2_Inode_RegFile))
		return -ENOTSUP;

	// TODO: Check `O_LARGEFILE`

	return 0;
}

static ssize_t ext2_f_read(struct file *f, void *buf, size_t n)
{
	const struct ext2_inode *const ino = f->f_inode->i_prv;

	return ext2_inode_read(f->f_inode->i_sb->s_bdev, ino, (size_t)f->f_pos, buf, n);
}

static off_t ext2_f_lseek(struct file *f, off_t off, int whence)
{
	const struct ext2_inode *const ino = f->f_inode->i_prv;
	off_t                          new_pos;

	switch (whence) {
	case SEEK_SET:
		new_pos = off;
		break;
	case SEEK_CUR:
		new_pos = f->f_pos + off;
		break;
	case SEEK_END:
		new_pos = (off_t)ino->i_size + off;
		break;
	default:
		return -EINVAL;
	}

	if (new_pos < 0)
		return -EINVAL;

	f->f_pos = new_pos;

	return new_pos;
}

static const struct file_operations ext2_fops = {
    .open  = ext2_f_open,
    .read  = ext2_f_read,
    .lseek = ext2_f_lseek,
};

static int ext2_dir_open(struct file *) { return 0; }

static int ext2_dir_read(struct file *, void *, size_t) { return -ENOTSUP; }

static off_t ext2_dir_lseek(struct file *, off_t, int) { return -ENOTSUP; }

static int ext2_dtype(uint8_t file_type)
{
	switch (file_type) {
	case 1: /* EXT2_FT_REG_FILE */
		return DT_REG;
	case 2: /* EXT2_FT_DIR */
		return DT_DIR;
	case 3: /* EXT2_FT_CHRDEV */
		return DT_CHR;
	case 4: /* EXT2_FT_BLKDEV */
		return DT_BLK;
	case 5: /* EXT2_FT_FIFO */
		return DT_FIFO;
	case 6: /* EXT2_FT_SOCK */
		return DT_SOCK;
	case 7: /* EXT2_FT_SYMLINK */
		return DT_LNK;
	default:
		return DT_UNKNOWN;
	}
}

static int ext2_iterate(struct file *f, struct dirctx *ctx)
{
	assert(f && ctx);

	const struct ext2_inode *const ino = f->f_inode->i_prv;

	if (!EXT2_InodeTypeCheck(*ino, Ext2_Inode_Dir))
		return -ENOTDIR;

	struct block_device *const dev = f->f_inode->i_sb->s_bdev;
	const size_t               bs  = dev->block_size;

	while (ctx->pos < (off_t)ino->i_size) {
		const size_t blk_idx = (size_t)ctx->pos / bs;
		size_t       off     = (size_t)ctx->pos % bs;
		uint32_t     blk;

		int ret = ext2_inode_bmap(dev, ino, blk_idx, &blk);
		if (ret < 0)
			return ret;
		if (!blk)
			return -EIO; /* directories must not have holes */

		struct bcache *b = bread(dev, blk);
		if (!b)
			return -EIO;

		while (off < bs) {
			const struct ext2_dentry *de = (void *)(b->data + off);

			if (!ext2_dentry_valid(de, bs - off)) {
				brelse(b);
				return -EIO;
			}

			if (de->inode &&
			    ctx->emit(de->name, de->name_len, de->inode, ext2_dtype(de->file_type))) {
				brelse(b);
				return 1; /* sink full, `pos` stays on this entry */
			}

			off += de->rec_len;
			ctx->pos += de->rec_len;
		}

		brelse(b);
	}

	return 0;
}

static const struct file_operations ext2_dir_ops = {
    .open    = ext2_dir_open,
    .read    = ext2_dir_read,
    .lseek   = ext2_dir_lseek,
    .iterate = ext2_iterate,
};

static struct dentry *ext2_i_lookup(struct inode *parent, struct dentry *child)
{
	assert(parent && child);

	struct ext2_inode *resolved =
	    kmalloc(sizeof(struct ext2_inode) + sizeof(struct inode), GFP_KERNEL);

	if (unlikely(!resolved))
		return ERR_PTR(-ENOMEM);

	int ret = ext2_find_inode(parent->i_sb, parent->i_prv, child->d_name, ft_strlen(child->d_name),
	                          resolved);
	if (ret < 0) {
		kfree(resolved);
		return ERR_PTR(ret);
	}

	struct inode *ino = (struct inode *)(resolved + 1);

	ino->i_mode  = resolved->i_mode;
	ino->i_size  = resolved->i_size;
	ino->i_sb    = parent->i_sb;
	ino->i_count = 1;
	ino->i_op    = ino->i_sb->s_i_op;
	ino->i_f_op  = (EXT2_InodeTypeCheck(*resolved, Ext2_Inode_Dir)) ? &ext2_dir_ops : &ext2_fops;
	ino->i_prv   = resolved;
	INIT_SENTINEL(&ino->i_dentry);

	child->d_inode = ino;

	return child;
}

/* `i_prv` is the start of the allocation made by `ext2_i_lookup`, the inode lives inside it */
static void ext2_i_destroy(struct inode *ino) { kfree(ino->i_prv); }

static const struct inode_operations ext2_iops = {
    .lookup  = ext2_i_lookup,
    .destroy = ext2_i_destroy,
};

int ext2_parse_superblock(struct block_device *dev)
{
	assert(dev);

	const size_t old_block_size = dev->block_size;

	struct ext2_super_block *_sb =
	    kmalloc(sizeof(struct ext2_super_block) + sizeof(struct ext2_inode) +
	                sizeof(struct superblock) + sizeof(struct dentry) + sizeof(struct inode) + 2,
	            GFP_KERNEL);

	struct ext2_inode *_ino = (struct ext2_inode *)(_sb + 1);
	struct superblock *sb   = (struct superblock *)(_ino + 1);

	int ret = -ENOMEM;
	if (unlikely(!_sb))
		goto err;

	ret = blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE);
	if (ret == 0)
		ret = blkdev_read(dev, 1, 1, (void *)_sb);
	if (ret < 0)
		goto err;

	ret = -EINVAL;

	if (_sb->s_magic != EXT2_MAGIC) {
		vga_printf("ext2load: Not an ext2 filesystem (invalid magic)\n");
		goto err;
	}

	if (_sb->s_log_block_size > EXT2_MAX_LOG_BLOCK_SIZE) {
		vga_printf("ext2load: Not handled block size (log %u), aborting...\n",
		           _sb->s_log_block_size);
		goto err;
	}

	if (((_sb->s_rev_level << 8) | _sb->s_minor_rev_level) != 0x0100) {
		vga_printf("ext2load: Not handled Ext2 Rev. (%u.%u), aborting...\n", _sb->s_rev_level,
		           _sb->s_minor_rev_level);
		goto err;
	}

	if (_sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED) {
		vga_printf("ext2load: Not handled incompat features (0x%x), aborting...\n",
		           _sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED);
		goto err;
	}

	if (_sb->s_inode_size != sizeof(struct ext2_inode)) {
		vga_printf("ext2load: Not handled inode size (%u), aborting...\n", _sb->s_inode_size);
		goto err;
	}

	if (_sb->s_inodes_per_group == 0) {
		vga_printf("ext2load: Invalid inodes per group (0), aborting...\n");
		goto err;
	}

	ret = blkdev_set_block_size(dev, EXT2_INITIAL_BLOCK_SIZE << _sb->s_log_block_size);
	if (ret < 0)
		goto err;

	ret = ext2_read_inode(dev, _sb, EXT2_ROOT_INODE, _ino);
	if (ret < 0)
		goto err;

	if (!EXT2_InodeTypeCheck(*_ino, Ext2_Inode_Dir)) {
		vga_printf("ext2load: Root inode is not a directory, aborting...\n");
		ret = -EINVAL;
		goto err;
	}

	sb->s_root          = (struct dentry *)(sb + 1);
	sb->s_root->d_inode = (struct inode *)(sb->s_root + 1);
	sb->s_root->d_name  = (char *)(sb->s_root->d_inode + 1);

	sb->s_fs_info = _sb;
	sb->s_bdev    = dev;
	sb->s_type    = vfs_types[FS_EXT2];

	sb->s_i_op = &ext2_iops;

	ft_memcpy((char *)sb->s_root->d_name, "/", 2);

	sb->s_root->d_parent = sb->s_root;
	sb->s_root->d_sb     = sb;
	sb->s_root->d_count  = 1; /* pinned by the superblock */
	sb->s_root->d_hash   = (struct hlist_node){0};

	sb->s_root->d_inode->i_prv   = _ino;
	sb->s_root->d_inode->i_count = 1; /* pinned by the root dentry */
	sb->s_root->d_inode->i_sb    = sb;
	sb->s_root->d_inode->i_mode  = _ino->i_mode;
	sb->s_root->d_inode->i_size  = _ino->i_size;
	sb->s_root->d_inode->i_op    = sb->s_i_op;
	sb->s_root->d_inode->i_f_op  = &ext2_dir_ops;

	INIT_SENTINEL(&sb->s_root->d_child);
	INIT_SENTINEL(&sb->s_root->d_sibling);
	INIT_SENTINEL(&sb->s_root->d_inode->i_dentry);

	// TODO: Change it when implementing mounts, nothing is refcounted yet so the
	// old superblock is assumed to be unreferenced once replaced
	struct superblock *old_sb = root_sb;
	root_sb                   = sb;

	if (old_sb) {
		kfree(old_sb);
	}

	return 0;

err:
	blkdev_set_block_size(dev, old_block_size);
	kfree(_sb);

	return ret;
}
