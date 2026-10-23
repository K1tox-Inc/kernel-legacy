#pragma once

#include <kernel/block.h>
#include <kernel/hashtable.h>

struct bcache {
	struct block_device *bdev;
	uint32_t             blkno;
	char                *data;
	int                  refcnt;
	bool                 valid;
	struct hlist_node    hash;
	struct list_head     lru;
};

extern struct bcache *bread(struct block_device *bdev, uint32_t blkno);
extern void           brelse(struct bcache *b);
