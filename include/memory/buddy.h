#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <list.h>
#include <memory/memory.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

#define MAX_ORDER     10
#define MAX_MIGRATION 1

// Macros

#define PAGE_BY_ORDER(order)  (1 << order)
#define ORDER_TO_BYTES(order) (PAGE_BY_ORDER(order) * PAGE_SIZE)

// ============================================================================
// STRUCT
// ============================================================================

// Enums

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

// Structures

struct buddy_free_area {
	struct list_head free_list[MAX_MIGRATION];
	uint32_t         nr_free;
};

struct buddy_allocator {
	struct buddy_free_area areas[MAX_ORDER + 1];
};

// Typedefs

// Structures

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void   debug_buddy(void);
void   buddy_print_summary(void);
size_t buddy_print(enum zone_type zone);
