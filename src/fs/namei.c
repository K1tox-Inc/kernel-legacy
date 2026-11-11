#include <fs/dcache.h>
#include <fs/vfs.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <uapi/errno.h>
#include <utils/compiler.h>
#include <vga/vga.h>

/* Resolves `name` in `dir`. Always consumes the reference on `dir`, returns one on the result */
static struct dentry *walk_component(struct dentry *dir, const char *name)
{
	struct dentry *next;

	if (!ft_strncmp(name, ".", 2))
		return dir;

	if (!ft_strncmp(name, "..", 3)) {
		if (dir == dir->d_sb->s_root)
			return dir;
		next = dget(dir->d_parent);
		dput(dir);
		return next;
	}

	const struct inode_operations *i_op = dir->d_inode->i_op;

	if (unlikely(!IS_DIR(dir->d_inode) || !i_op || !i_op->lookup)) {
		dput(dir);
		return ERR_PTR(-ENOTDIR);
	}

	next = d_lookup(dir, name);

	if (!next) {
		next = d_alloc(dir, name);

		if (!IS_ERR(next)) {
			struct dentry *res = i_op->lookup(dir->d_inode, next);

			if (IS_ERR(res)) {
				d_free(next);
				next = res;
			} else {
				d_add(next);
			}
		}
	}

	dput(dir);

	return next;
}

static const char *splice_link(struct dentry *link, const char *rest, char *pbuf, char *lbuf)
{
	const ssize_t tlen = vfs_readlink(link->d_inode, lbuf, PATH_MAX);

	if (tlen < 0)
		return ERR_PTR(tlen);

	if (unlikely(tlen == 0))
		return ERR_PTR(-ENOENT);

	const size_t rlen = ft_strlen(rest);

	if (unlikely((size_t)tlen + rlen + 1 > PATH_MAX))
		return ERR_PTR(-ENAMETOOLONG);

	ft_memmove(pbuf + tlen, rest, rlen + 1);
	ft_memcpy(pbuf, lbuf, (size_t)tlen);

	return pbuf;
}

struct dentry *vfs_lookup(const char *path, struct dentry *start, unsigned int flags)
{
	assert(path && start);

	if (unlikely(!root_sb))
		return ERR_PTR(-ENODEV);

	struct dentry *cur    = dget(start);
	char          *pbuf   = NULL;
	int            nlinks = 0;
	char           name[NAME_MAX + 1];

	while (*path) {
		while (*path == '/')
			path++;

		size_t len = 0;
		while (path[len] && path[len] != '/')
			len++;

		if (len == 0)
			break;

		if (unlikely(len > NAME_MAX)) {
			dput(cur);
			cur = ERR_PTR(-ENAMETOOLONG);
			goto out;
		}

		ft_memcpy(name, path, len);
		name[len] = '\0';
		path += len;

		struct dentry *parent = dget(cur);

		cur = walk_component(cur, name);

		if (IS_ERR(cur) || !IS_LNK(cur->d_inode) || (!*path && !(flags & LOOKUP_FOLLOW))) {
			dput(parent);
			if (IS_ERR(cur))
				goto out;
			continue;
		}

		if (unlikely(++nlinks > MAXSYMLINKS)) {
			dput(cur);
			dput(parent);
			cur = ERR_PTR(-ELOOP);
			goto out;
		}

		if (!pbuf) {
			pbuf = kmalloc(2 * PATH_MAX, GFP_KERNEL);

			if (unlikely(!pbuf)) {
				dput(cur);
				dput(parent);
				cur = ERR_PTR(-ENOMEM);
				goto out;
			}
		}

		path = splice_link(cur, path, pbuf, pbuf + PATH_MAX);
		dput(cur);

		if (IS_ERR(path)) {
			dput(parent);
			cur = ERR_PTR(PTR_ERR(path));
			goto out;
		}

		if (*path == '/') {
			dput(parent);
			cur = dget(root_sb->s_root);
		} else {
			cur = parent;
		}
	}

out:
	kfree(pbuf);
	return cur;
}

ssize_t vfs_readlink(struct inode *ino, char *buf, size_t len)
{
	assert(ino && buf);

	if (unlikely(!IS_LNK(ino)))
		return -EINVAL;

	const struct inode_operations *i_op = ino->i_op;

	if (unlikely(!i_op || !i_op->readlink))
		return -EINVAL;

	return i_op->readlink(ino, buf, len);
}
