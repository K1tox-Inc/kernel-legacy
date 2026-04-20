#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vma.h>
#include <utils/kmacro.h>

// ============================================================================
// INTERNAL APIs
// ============================================================================

void merge_neighbor(struct vm_area *start_area, struct vm_area *next_area)
{
	if (start_area->state != VM_AREA_FREE || next_area->state != VM_AREA_FREE)
		return;
	else if (vma_areas_are_neighbors(start_area, next_area)) {
		start_area->size += next_area->size;
		pop_node(&next_area->vma_node);
		kfree(next_area);
	}
}

// ============================================================================
// EXTERNAL APIs
// ============================================================================

struct vm_area *vma_first_fit_alloc(struct list_head *head, size_t size)
{
	struct vm_area *area;
	list_for_each_entry(area, head, vma_node)
	{
		if (area->size >= size && area->state == VM_AREA_FREE)
			return area;
	}
	return NULL;
}

struct vm_area *vma_split_area(struct vm_area *area, size_t size)
{
	size_t needed_size  = ALIGN(size, PAGE_SIZE);
	size_t num_of_pages = needed_size / PAGE_SIZE;

	if (needed_size > area->size)
		return NULL;
	else if (needed_size == area->size) {
		area->nr_pages = num_of_pages;
		area->state    = VM_AREA_ALLOCATED;
		return area;
	}

	struct vm_area *new_area = kmalloc(sizeof(struct vm_area), GFP_KERNEL);
	if (!new_area)
		return NULL;

	new_area->state    = VM_AREA_ALLOCATED;
	new_area->size     = needed_size;
	new_area->nr_pages = num_of_pages;

	new_area->start_vaddr = area->start_vaddr;
	area->start_vaddr += needed_size;
	area->size -= needed_size;

	list_insert(&new_area->vma_node, area->vma_node.prev, &area->vma_node);

	return new_area;
}

void vma_merge_area(struct list_head *head, struct vm_area *area)
{
	struct vm_area *next_area = list_entry(area->vma_node.next, struct vm_area, vma_node);
	struct vm_area *prev_area = list_entry(area->vma_node.prev, struct vm_area, vma_node);

	if (&next_area->vma_node != head)
		merge_neighbor(area, next_area);
	if (&prev_area->vma_node != head)
		merge_neighbor(prev_area, area);
}

struct vm_area *vma_find_area(void *ptr, struct list_head *head)
{
	if (!ptr)
		return NULL;
	struct vm_area *area = NULL;
	list_for_each_entry(area, head, vma_node)
	{
		if (area->start_vaddr == (uintptr_t)ptr)
			return area;
	}
	return NULL;
}

size_t vma_size(void *ptr, struct list_head *head)
{
	if (!ptr)
		return 0;
	struct vm_area *area = vma_find_area(ptr, head);
	if (area)
		return area->size;
	return 0;
}
