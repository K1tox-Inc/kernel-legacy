#pragma once

#include <types.h>

#define HMAP_1024 10 // 4KiB

#define GOLDEN_RATIO_32 0x61C88647u

#define hash_32(val, bits) ((val) * GOLDEN_RATIO_32) >> (32 - (bits))

#define DEFINE_HASHTABLE(name, bits) struct hlist_head name[1 << (bits)] = {0}

#define ilog2(x) (31 - __builtin_clz(x))

#define HASH_SIZE(name) (sizeof(name) / sizeof((name)[0]))
#define HASH_BITS(name) ilog2(HASH_SIZE(name))

#define hash_add(table, node, key) hlist_add_head(node, &(table)[hash_32(key, HASH_BITS(table))])

#define hash_del(node) hlist_del(node)

#define hash_for_each_possible(table, obj, member, key)                                            \
	hlist_for_each_entry(obj, &(table)[hash_32(key, HASH_BITS(table))], member)

struct hlist_node {
	struct hlist_node *next, **pprev;
};

struct hlist_head {
	struct hlist_node *first;
};

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	n->next = h->first;
	if (h->first)
		h->first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline void hlist_del(struct hlist_node *n)
{
	*n->pprev = n->next; /* whoever pointed at n now skips it */
	if (n->next)
		n->next->pprev = n->pprev;
	n->next  = NULL;
	n->pprev = NULL;
}

static inline bool hlist_unhashed(const struct hlist_node *n) { return !n->pprev; }

#define hlist_for_each_entry(pos, head, member)                                                    \
	for ((pos) = (head)->first ? container_of((head)->first, typeof(*(pos)), member) : NULL; pos;  \
	     (pos) = (pos)->member.next ? container_of((pos)->member.next, typeof(*(pos)), member)     \
	                                : NULL)
