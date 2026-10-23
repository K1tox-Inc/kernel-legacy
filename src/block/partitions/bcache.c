#include <kernel/bcache.h>
#include <kernel/block.h>
#include <kernel/hashtable.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>

#define BCACHE_HMAP_BITS HMAP_1024
static DEFINE_HASHTABLE(bcache, BCACHE_HMAP_BITS);

static inline uint32_t bcache_hash(struct block_device *bdev, uint32_t blkno)
{
	return hash_32(blkno ^ ((uintptr_t)bdev >> 4), BCACHE_HMAP_BITS);
}

static struct bcache *cache_lookup(struct block_device *bdev, uint32_t blkno)
{
	uint32_t       key = blkno ^ ((uintptr_t)bdev >> 4);
	struct bcache *b;

	hash_for_each_possible(bcache, b, hash, key)
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
			return NULL;
		b->bdev   = bdev;
		b->blkno  = blkno;
		b->data   = (void *)(b + 1);
		b->refcnt = 0;
		b->valid  = false;
		hash_add(bcache, &b->hash, bcache_hash(bdev, blkno));
	}
	b->refcnt++;
	if (!b->valid) {
		if (blkdev_read(bdev, blkno, 1, b->data) < 0) {
			brelse(b);
			return NULL;
		}
		b->valid = true;
	}
	return b;
}

void brelse(struct bcache *b)
{
	if (--b->refcnt == 0) {
		hash_del(&b->hash);
		kfree(b);
	}
}
