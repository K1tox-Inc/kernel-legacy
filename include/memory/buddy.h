#pragma once

#include <list.h>
#include <memory/memory.h>
#include <types.h>

#define MAX_ORDER     10
#define MAX_MIGRATION 1

#define PAGE_BY_ORDER(order)  (1 << (order))
#define ORDER_TO_BYTES(order) (PAGE_BY_ORDER(order) * PAGE_SIZE)

enum order_size {
	ORDER_4KIB = 0,
	ORDER_8KIB,
	ORDER_16KIB,
	ORDER_32KIB,
	ORDER_64KIB,
	ORDER_128KIB,
	ORDER_256KIB,
	ORDER_512KIB,
	ORDER_1MIB,
	ORDER_2MIB,
	ORDER_4MIB,
	BAD_ORDER,
};

struct buddy_free_area {
	struct list_head free_list[MAX_MIGRATION];
	uint32_t         nr_free;
};

struct buddy_allocator {
	struct buddy_free_area areas[MAX_ORDER + 1];
};

void   debug_buddy(void);
void   buddy_print_summary(void);
size_t buddy_print(enum zone_type zone);
