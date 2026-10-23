#include <fs/vfs.h>
#include <kernel/hashtable.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <types.h>
#include <uapi/errno.h>
#include <utils/assert.h>
#include <utils/compiler.h>

#define DCACHE_HMAP_BITS HMAP_1024
static DEFINE_HASHTABLE(dcache, DCACHE_HMAP_BITS);

static inline uint32_t d_hash_key(const struct dentry *parent, const char *name)
{
	uint32_t h = 5381;

	for (; *name; name++)
		h = h * 33 + (uint8_t)*name;

	return h ^ ((uintptr_t)parent >> 4);
}

struct dentry *dget(struct dentry *d)
{
	assert(d);

	d->d_count++;

	return d;
}

void dput(struct dentry *d)
{
	assert(d && d->d_count > 0);

	// TODO: Unused dentries (`d_count == 0`) stay cached, evict them once there is a shrinker
	d->d_count--;
}

struct dentry *d_alloc(struct dentry *parent, const char *name)
{
	if (unlikely(!parent || !name))
		return ERR_PTR(-EINVAL);

	size_t         name_len = ft_strlen(name);
	struct dentry *d        = kmalloc(sizeof(struct dentry) + name_len + 1, GFP_KERNEL);

	if (unlikely(!d))
		return ERR_PTR(-ENOMEM);

	d->d_name = (char *)(d + 1);
	ft_memcpy((char *)d->d_name, name, name_len + 1);

	d->d_parent = dget(parent);
	d->d_sb     = parent->d_sb;
	d->d_inode  = NULL;
	d->d_count  = 1;
	d->d_hash   = (struct hlist_node){0};

	INIT_SENTINEL(&d->d_child);
	INIT_SENTINEL(&d->d_sibling);

	return d;
}

void d_add(struct dentry *d)
{
	assert(d && d->d_inode && hlist_unhashed(&d->d_hash));

	list_add_tail(&d->d_sibling, &d->d_parent->d_child);
	hash_add(dcache, &d->d_hash, d_hash_key(d->d_parent, d->d_name));
}

void d_free(struct dentry *d)
{
	assert(d && d->d_count == 1 && hlist_unhashed(&d->d_hash));

	if (d->d_inode)
		iput(d->d_inode);

	dput(d->d_parent);
	kfree(d);
}

struct dentry *d_lookup(const struct dentry *parent, const char *name)
{
	assert(parent && name);

	struct dentry *d;

	hash_for_each_possible(dcache, d, d_hash, d_hash_key(parent, name))
	{
		if (d->d_parent == parent && ft_strequ(d->d_name, name))
			return dget(d);
	}

	return NULL;
}
