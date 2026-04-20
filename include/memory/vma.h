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

enum vm_area_state { VM_AREA_FREE, VM_AREA_ALLOCATED };

struct vm_area {
	struct list_head   vma_node;
	enum vm_area_state state;
	uintptr_t          start_vaddr;
	size_t             size;

	// Only initialized when state is VM_AREA_ALLOCATED otherwise set as NULL
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

size_t          vma_size(void *ptr, struct list_head *head);
struct vm_area *vma_split_area(struct vm_area *area, size_t size);
struct vm_area *vma_first_fit_alloc(struct list_head *head, size_t size);
void            vma_merge_area(struct list_head *head, struct vm_area *area);
struct vm_area *vma_find_area(void *ptr, struct list_head *head);
