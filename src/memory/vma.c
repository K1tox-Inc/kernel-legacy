#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vma.h>
#include <utils/kmacro.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>

// ============================================================================
// INTERNAL APIs
// ============================================================================

static void merge_neighbor(struct vm_area *start_area, struct vm_area *next_area)
{
	if (start_area->state != VM_AREA_FREE || next_area->state != VM_AREA_FREE)
		return;
	else if (vma_areas_are_neighbors(start_area, next_area)) {
		start_area->size += next_area->size;
		pop_node(&next_area->vma_node);
		kfree(next_area);
	}
}

static struct vm_area *vma_alloc_in_area(struct list_head *head, uintptr_t pd, struct vm_area *area, size_t size, uint32_t pte_flags)
{
	struct vm_area *new_area = vma_split_area(area, size);
	if (!new_area)
		return NULL;

	size_t number_of_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	new_area->pages  = kmalloc(sizeof(uintptr_t) * number_of_pages, GFP_KERNEL);
	if (!new_area->pages)
		goto free_allocated_area;
	new_area->nr_pages = number_of_pages;

	size_t num_of_alloc = 0;
	for (size_t i = 0; i < number_of_pages; i++, num_of_alloc++) {
		void *page = buddy_alloc_pages(PAGE_SIZE, HIGHMEM_ZONE);
		if (!page)
			goto free_allocated_pages;
		new_area->pages[i] = (uintptr_t)page;
	}

	for (size_t i = 0; i < number_of_pages; i++) {
		uintptr_t paddr = new_area->pages[i];
		uintptr_t vaddr = new_area->start_vaddr + (i * PAGE_SIZE);
		// Can fail only if buddy is out of memory, im lazy to implement the unmap label for this
		// case
		if (!vmm_map_page(pd, vaddr, paddr, pte_flags))
			kpanic("vmalloc: vmm_map_page failed!");
	}

	return new_area;

free_allocated_pages:
	for (size_t i = 0; i < num_of_alloc; i++)
		buddy_free_block((void *)new_area->pages[i]);
	kfree(new_area->pages);
free_allocated_area:
	area->state = VM_AREA_FREE;
	vma_merge_area(head, new_area);

	return NULL;
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

struct vm_area *vma_find_by_start(void *ptr, struct list_head *head)
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

struct vm_area *vma_find_by_addr(void *ptr, struct list_head *head)
{
    if (!ptr)
        return NULL;
    struct vm_area *area;
    list_for_each_entry(area, head, vma_node) {
        if ((uintptr_t)ptr >= area->start_vaddr &&
            (uintptr_t)ptr < area->start_vaddr + area->size)
            return area;
    }
    return NULL;
}

size_t vma_size(void *ptr, struct list_head *head)
{
	if (!ptr)
		return 0;
	struct vm_area *area = vma_find_by_start(ptr, head);
	if (area)
		return area->size;
	return 0;
}

void vma_init_area(struct list_head *head, uintptr_t start, uintptr_t end)
{
    struct vm_area *initial_hole = kmalloc(sizeof(struct vm_area), GFP_KERNEL);
    if (!initial_hole)
        kpanic("vma_init_area failed!");

    *initial_hole = (struct vm_area){
        .state       = VM_AREA_FREE,
        .start_vaddr = start,
        .size        = end - start,
        .nr_pages    = 0,
        .pages       = NULL,
    };

    INIT_SENTINEL(head);
    list_add_head(&initial_hole->vma_node, head);
}

struct vm_area *vma_alloc(struct list_head *head, uintptr_t pd, size_t size, uint32_t pte_flags, void *hint_vaddr)
{

	struct vm_area *free_area  = vma_find_by_start(hint_vaddr, head);
	if (!free_area || free_area->state != VM_AREA_FREE || free_area->size < size)
		free_area = vma_first_fit_alloc(head, size);
	if (!free_area)
		return NULL;
	
	return vma_alloc_in_area(head, pd, free_area, size, pte_flags);
}