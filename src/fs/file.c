#include <fs/dcache.h>
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include <uapi/errno.h>
#include <uapi/fcntl.h>
#include <utils/compiler.h>

int vfs_open(const char *path, int flags, struct file **out)
{
	if (!root_sb)
		return -ENODEV;

	assert(path && out);

	if (flags & (O_RDWR | O_CREAT | O_TRUNC))
		return -EROFS;

	struct dentry *d = vfs_lookup(path, root_sb->s_root);

	if (IS_ERR(d))
		return PTR_ERR(d);

	int ret;

	if (IS_DIR(d->d_inode) && !(flags & O_DIRECTORY)) {
		ret = -EISDIR;
		goto err_put;
	}

	if (!IS_DIR(d->d_inode) && flags & O_DIRECTORY) {
		ret = -ENOTDIR;
		goto err_put;
	}

	struct file *f = kmalloc(sizeof(struct file), GFP_KERNEL);

	if (unlikely(!f)) {
		ret = -ENOMEM;
		goto err_put;
	}

	/* The reference taken by `vfs_lookup` now belongs to the file */
	f->f_pos    = 0;
	f->f_flags  = flags;
	f->f_dentry = d;
	f->f_inode  = d->d_inode;
	f->f_op     = f->f_inode->i_f_op;

	if (f->f_op && f->f_op->open) {
		ret = f->f_op->open(f);

		if (ret < 0) {
			kfree(f);
			goto err_put;
		}
	}

	*out = f;

	return 0;

err_put:
	dput(d);

	return ret;
}

int vfs_close(struct file *f)
{
	assert(f);

	int ret = 0;

	if (f->f_op && f->f_op->release)
		ret = f->f_op->release(f);

	dput(f->f_dentry);
	kfree(f);

	return ret;
}

ssize_t vfs_read(struct file *f, void *buf, size_t n)
{
	assert(f);

	if (unlikely(!f->f_op || !f->f_op->read))
		return -EINVAL;

	ssize_t ret = f->f_op->read(f, buf, n);

	if (ret > 0)
		f->f_pos += ret;

	return ret;
}

ssize_t vfs_write(struct file *f, __always_unused const void *buf, __always_unused size_t n)
{
	assert(f);

	if (unlikely(!f->f_op || !f->f_op->write))
		return -EINVAL;

	return -EROFS;
}

off_t vfs_lseek(struct file *f, off_t off, int whence)
{
	assert(f);

	if (unlikely(!f->f_op || !f->f_op->lseek))
		return -EINVAL;

	return f->f_op->lseek(f, off, whence);
}

int vfs_iterate(struct file *f, struct dirctx *ctx)
{
	if (unlikely(!f || !ctx))
		return -EINVAL;

	if (unlikely(!f->f_op || !f->f_op->iterate))
		return -ENOTDIR;

	ctx->pos = f->f_pos;

	const int ret = f->f_op->iterate(f, ctx);

	f->f_pos = ctx->pos;

	return ret;
}
