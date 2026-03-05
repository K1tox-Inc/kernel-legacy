#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <types.h>
#include <utils/kmacro.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

#define MAX_PAGES  (1UL << 20)
#define PAGE_MAGIC 0xDEADBEEF

// Page mask
#define PAGE_STATE_MASK 0b00000111 // Bits 0-2 For page state
#define PAGE_ZONE_MASK  0b00111000 // Bits 3-5 for memory zone

// Satate
#define PAGE_STATE_UNUSABLE  0b00000000 // Hole
#define PAGE_STATE_RESERVED  0b00000001 // Reserved by bootstrap allocator
#define PAGE_STATE_AVAILABLE 0b00000010 // Usable not claimed
#define PAGE_STATE_FREE      0b00000011 // Available in buddy free list
#define PAGE_STATE_ALLOCATED 0b00000100 // Allocated by buddy
#define PAGE_STATE_SLAB      0b00000110 // Allocated by slab

// Memory Zones
#define PAGE_ZONE_DMA     0b00001000
#define PAGE_ZONE_LOWMEM  0b00010000
#define PAGE_ZONE_HIGHMEM 0b00100000

// Getters
#define PAGE_GET_STATE(page) ((page)->flags & PAGE_STATE_MASK)
#define PAGE_GET_ZONE(page)  ((page)->flags & PAGE_ZONE_MASK)
// Setters
#define PAGE_SET_STATE(page, state) ((page)->flags = ((page)->flags & ~PAGE_STATE_MASK) | (state))

// Checkers
#define PAGE_IS_FREE(page)      (PAGE_GET_STATE(page) == PAGE_STATE_FREE)
#define PAGE_IS_ALLOCATED(page) (PAGE_GET_STATE(page) == PAGE_STATE_ALLOCATED)

// Macros

// ============================================================================
// STRUCT
// ============================================================================

// Enums

enum direction { NEXT = 0, PREV };

// Structures

struct page {
	struct list_head node;
	uint32_t         flags;
	uintptr_t        private_data;
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

extern uint32_t     total_pages;
extern uint32_t     total_RAM;
extern struct page *page_descriptors;

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void         page_print_info(struct page *page);
void         page_descriptor_init(void);
uint32_t     page_to_index(struct page *page);
uintptr_t    page_to_phys(struct page *page);
struct page *page_index_to_page(uint32_t idx);
struct page *page_addr_to_page(uintptr_t addr);
struct page *page_addr_to_usable(uintptr_t addr, enum direction direction);
uint32_t     page_get_updated_reserved_count(void);
uint32_t     page_get_updated_free_count(void);
uint32_t     page_get_free_count(void);
uint32_t     page_get_reserved_count(void);
bool         page_addr_is_same_page(uintptr_t addr1, uintptr_t addr2);
