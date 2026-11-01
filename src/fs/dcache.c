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

static __always_inline uint32_t dcache_hash(const struct dentry *parent, const char *name)
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
	d->d_hnode  = (struct hlist_node){0};

	INIT_SENTINEL(&d->d_child);
	INIT_SENTINEL(&d->d_sibling);

	return d;
}

void d_add(struct dentry *d)
{
	assert(d && d->d_inode && hlist_unhashed(&d->d_hnode));

	list_add_tail(&d->d_sibling, &d->d_parent->d_child);
	hash_add(dcache, &d->d_hnode, dcache_hash(d->d_parent, d->d_name));
}

void d_free(struct dentry *d)
{
	assert(d && d->d_count == 1 && hlist_unhashed(&d->d_hnode));

	if (d->d_inode)
		iput(d->d_inode);

	dput(d->d_parent);
	kfree(d);
}

struct dentry *d_lookup(const struct dentry *parent, const char *name)
{
	assert(parent && name);

	struct dentry *d;

	hash_for_each_possible(dcache, d, d_hnode, dcache_hash(parent, name))
	{
		if (d->d_parent == parent && ft_strequ(d->d_name, name))
			return dget(d);
	}

	return NULL;
}

/* A child holds a reference on its parent, so an unused dentry has one count per child */
static bool d_subtree_busy(struct dentry *d)
{
	struct dentry *child;
	int            nr_children = 0;

	list_for_each_entry(child, &d->d_child, d_sibling)
	{
		if (d_subtree_busy(child))
			return true;
		nr_children++;
	}

	return d->d_count != nr_children;
}

static void d_prune_children(struct dentry *d)
{
	struct list_head *pos, *tmp;

	list_for_each_safe(pos, tmp, &d->d_child)
	{
		struct dentry *child = list_entry(pos, struct dentry, d_sibling);

		d_prune_children(child);

		pop_node(&child->d_sibling);
		hash_del(&child->d_hnode);

		if (child->d_inode)
			iput(child->d_inode);

		d->d_count--;
		kfree(child);
	}
}

int d_prune(struct dentry *d)
{
	assert(d);

	struct dentry *child;

	list_for_each_entry(child, &d->d_child, d_sibling)
	{
		if (d_subtree_busy(child))
			return -EBUSY;
	}

	d_prune_children(d);

	return 0;
}
