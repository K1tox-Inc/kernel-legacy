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

struct region_s {
	uintptr_t start;
	uintptr_t end;
};

// Typedefs

typedef struct region_s       region_t;
typedef struct boot_allocator boot_allocator_t;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

uint32_t  boot_allocator_get_res_zones_count(int type);
uint32_t  boot_allocator_get_free_zones_count(int type);
uint32_t  boot_allocator_get_region_count(enum mem_type type);
region_t *boot_allocator_get_res_zone(int type);
region_t *boot_allocator_get_free_zone(int type);
region_t *boot_allocator_get_region(enum mem_type type);
void     *boot_alloc(uint32_t size, zone_type zone, bool freeable);
void *boot_alloc_at(uint32_t size, zone_type zone, bool freeable, uintptr_t start, uintptr_t end,
                    int align);
bool  boot_allocator_range_overlaps(uintptr_t start, uintptr_t end, enum mem_type type);
void  boot_allocator_freeze(void);
void  boot_allocator_print_inital_layout(void);
void  boot_allocator_res_zones_printer(void);
void  boot_allocator_free_zones_printer(void);
void  boot_allocator_init(multiboot_tag_mmap_t *mmap, uint8_t *mmap_end);
void  boot_alloc_clean_up(void);
void  boot_allocator_print_allocations(void);
