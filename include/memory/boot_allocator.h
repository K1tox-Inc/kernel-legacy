#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <kernel/mb2_info.h>
#include <memory/memory.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

// Macros

// ============================================================================
// STRUCT
// ============================================================================

// Enums

enum mem_type { FREE_MEMORY = 0, RESERVED_MEMORY, HOLES_MEMORY };

// Structures

struct region {
	uintptr_t start;
	uintptr_t end;
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

uint32_t       boot_allocator_get_reserved_zones_count(int type);
uint32_t       boot_allocator_get_free_zones_count(int type);
uint32_t       boot_allocator_get_regions_count(enum mem_type type);
struct region *boot_allocator_get_reserved_zones(int type);
struct region *boot_allocator_get_free_zones(int type);
struct region *boot_allocator_get_regions(enum mem_type type);
void          *boot_alloc(uint32_t size, enum zone_type zone, bool freeable);
void          *boot_alloc_at(uint32_t size, enum zone_type zone, bool freeable, uintptr_t start,
                             uintptr_t end, int align);
bool           boot_allocator_range_overlaps(uintptr_t start, uintptr_t end, enum mem_type type);
void           boot_allocator_freeze(void);
void           boot_allocator_print_initial_layout(void);
void           boot_allocator_print_reserved_zones(void);
void           boot_allocator_print_free_zones(void);
void           boot_allocator_init(struct multiboot_tag_mmap *mmap, uint8_t *mmap_end);
void           boot_allocator_print_allocations(void);
