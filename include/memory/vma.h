#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <list.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// ============================================================================
// STRUCT
// ============================================================================

enum vm_area_state { VM_AREA_FREE, VM_AREA_ALLOCATED, VM_AREA_LAZY };
enum vma_alloc_mode { VMA_EAGER, VMA_LAZY };

struct vm_area {
	struct list_head   vma_node;
	enum vm_area_state state;
	uintptr_t          start_vaddr;
	size_t             size;

	// Initialized for allocated and lazy areas; NULL for free/uninitialized areas.
	uint32_t   pte_flags;
	size_t     nr_pages;
	uintptr_t *pages;
};

static inline bool vma_areas_are_neighbors(struct vm_area *start_area, struct vm_area *next_area)
{
	return (start_area->start_vaddr + start_area->size == next_area->start_vaddr);
}

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void            vma_print_areas(struct list_head *head);
void            vma_merge_area(struct list_head *head, struct vm_area *area);
void            vma_init_area(struct list_head *head, uintptr_t start, uintptr_t end);
void            vma_destroy_area(struct list_head *head, struct vm_area *area, uintptr_t pd);
bool            vma_map_area(struct vm_area *new_area, uintptr_t pd);
size_t          vma_size(void *ptr, struct list_head *head);
struct vm_area *vma_split_area(struct vm_area *area, size_t size);
struct vm_area *vma_first_fit_alloc(struct list_head *head, size_t size);
struct vm_area *vma_find_by_addr(void *ptr, struct list_head *head);
struct vm_area *vma_find_by_start(void *ptr, struct list_head *head);
struct vm_area *vma_alloc(struct list_head *head, uintptr_t pd, size_t size, uint32_t pte_flags,
                          void *hint_vaddr, enum vma_alloc_mode);
