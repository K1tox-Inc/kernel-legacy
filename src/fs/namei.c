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
		/* `..` of the filesystem root is the root itself */
		next = dget(dir == dir->d_sb->s_root ? dir : dir->d_parent);
		dput(dir);
		return next;
	}

	if (unlikely(!IS_DIR(dir->d_inode))) {
		dput(dir);
		return ERR_PTR(-ENOTDIR);
	}

	next = d_lookup(dir, name);

	if (!next) {
		next = d_alloc(dir, name);

		if (!IS_ERR(next)) {
			struct dentry *res = dir->d_inode->i_op->lookup(dir->d_inode, next);

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

struct dentry *vfs_lookup(const char *path, struct dentry *start)
{
	assert(path && start);

	if (unlikely(!root_sb))
		return ERR_PTR(-ENODEV);

	struct dentry *cur = dget(start);
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
			return ERR_PTR(-ENAMETOOLONG);
		}

		ft_memcpy(name, path, len);
		name[len] = '\0';
		path += len;

		cur = walk_component(cur, name);

		if (IS_ERR(cur))
			return cur;
	}

	return cur;
}
