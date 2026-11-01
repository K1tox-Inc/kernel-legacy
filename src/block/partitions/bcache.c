#include <kernel/bcache.h>
#include <kernel/block.h>
#include <kernel/hashtable.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>
#include <utils/compiler.h>

#define BCACHE_HMAP_BITS HMAP_1024
static DEFINE_HASHTABLE(bcache, BCACHE_HMAP_BITS);

static __always_inline uint32_t bcache_hash(struct block_device *bdev, uint32_t blkno)
{
	return blkno ^ ((uintptr_t)bdev >> 4);
}

static struct bcache *cache_lookup(struct block_device *bdev, uint32_t blkno)
{
	uint32_t       key = bcache_hash(bdev, blkno);
	struct bcache *b;

	hash_for_each_possible(bcache, b, hnode, key)
	{
		if (b->bdev == bdev && b->blkno == blkno)
			return b;
	}

	return NULL;
}

struct bcache *bread(struct block_device *bdev, uint32_t blkno)
{
	struct bcache *b = cache_lookup(bdev, blkno);
	if (!b) {
		// TODO: Change to `cache_alloc(bdev, blkno)` and evict an LRU buf with refcnt 0
		b = kmalloc(sizeof(struct bcache) + bdev->block_size, GFP_KERNEL);
		if (!b)
			return ERR_PTR(-ENOMEM);
		b->bdev   = bdev;
		b->blkno  = blkno;
		b->data   = (void *)(b + 1);
		b->refcnt = 0;
		b->valid  = false;
		hash_add(bcache, &b->hnode, bcache_hash(bdev, blkno));
	}

	b->refcnt++;
	if (!b->valid) {
		if (blkdev_read(bdev, blkno, 1, b->data) < 0) {
			brelse(b);
			return ERR_PTR(-EIO);
		}
		b->valid = true;
	}
	return b;
}

// TODO: Keep the buffer cached at `refcnt == 0` once `bread` can evict, every `bread` hits the
// disk until then
void brelse(struct bcache *b)
{
	if (--b->refcnt == 0) {
		hash_del(&b->hnode);
		kfree(b);
	}
}
