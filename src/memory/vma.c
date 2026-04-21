#include <kernel/panic.h>
#include <libk.h>
#include <list.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vma.h>
#include <memory/vmm.h>
#include <utils/kmacro.h>

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

static struct vm_area *vma_alloc_in_area(struct list_head *head, uintptr_t pd, struct vm_area *area,
                                         size_t size, uint32_t pte_flags,
                                         enum vma_alloc_mode alloc_mode)
{
	struct vm_area *new_area = vma_split_area(area, size);
	if (!new_area)
		return NULL;

	size_t number_of_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	new_area->pages        = kmalloc(sizeof(uintptr_t) * number_of_pages, GFP_KERNEL | __GFP_ZERO);
	if (!new_area->pages)
		goto free_on_error;
	new_area->nr_pages  = number_of_pages;
	new_area->pte_flags = pte_flags;

	if (alloc_mode == VMA_EAGER) {
		if (!vma_map_area(new_area, pd))
			goto free_on_error;
	} else {
		new_area->state = VM_AREA_LAZY;
	}

	return new_area;

free_on_error:
	vma_destroy_area(head, new_area, pd);
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
	size_t num_of_pages = DIV_ROUND_UP(needed_size, PAGE_SIZE);

	if (needed_size > area->size)
		return NULL;
	else if (needed_size == area->size) {
		area->nr_pages = num_of_pages;
		area->state    = VM_AREA_ALLOCATED;
		return area;
	}

	struct vm_area *new_area = kmalloc(sizeof(struct vm_area), GFP_KERNEL | __GFP_ZERO);
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
	if (area->vma_node.next != head) {
		struct vm_area *next_area = list_entry(area->vma_node.next, struct vm_area, vma_node);
		merge_neighbor(area, next_area);
	}

	if (area->vma_node.prev != head) {
		struct vm_area *prev_area = list_entry(area->vma_node.prev, struct vm_area, vma_node);
		merge_neighbor(prev_area, area);
	}
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
	list_for_each_entry(area, head, vma_node)
	{
		if ((uintptr_t)ptr >= area->start_vaddr && (uintptr_t)ptr < area->start_vaddr + area->size)
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
	    .pte_flags   = 0,
	};

	INIT_SENTINEL(head);
	list_add_head(&initial_hole->vma_node, head);
}

struct vm_area *vma_alloc(struct list_head *head, uintptr_t pd, size_t size, uint32_t pte_flags,
                          void *hint_vaddr, enum vma_alloc_mode alloc_mode)
{

	struct vm_area *free_area = vma_find_by_start(hint_vaddr, head);
	if (!free_area || free_area->state != VM_AREA_FREE || free_area->size < size)
		free_area = vma_first_fit_alloc(head, size);
	if (!free_area)
		return NULL;

	return vma_alloc_in_area(head, pd, free_area, size, pte_flags, alloc_mode);
}

void vma_destroy_area(struct list_head *head, struct vm_area *area, uintptr_t pd)
{
	for (size_t i = 0; i < area->nr_pages; i++) {
		if (!area->pages[i])
			continue;
		vmm_unmap_page(pd, area->start_vaddr + (i * PAGE_SIZE));
		buddy_free_block((void *)area->pages[i]);
	}

	kfree(area->pages);
	area->pages     = NULL;
	area->nr_pages  = 0;
	area->state     = VM_AREA_FREE;
	area->pte_flags = 0;
	vma_merge_area(head, area);
}

bool vma_map_area(struct vm_area *new_area, uintptr_t pd)
{
	size_t number_of_pages = new_area->nr_pages;
	for (size_t i = 0; i < number_of_pages; i++) {
		void *page = buddy_alloc_pages(PAGE_SIZE, HIGHMEM_ZONE);
		if (!page)
			return false;
		new_area->pages[i] = (uintptr_t)page;
	}

	for (size_t i = 0; i < number_of_pages; i++) {
		uintptr_t paddr = new_area->pages[i];
		uintptr_t vaddr = new_area->start_vaddr + (i * PAGE_SIZE);
		if (!vmm_map_page(pd, vaddr, paddr, new_area->pte_flags))
			return false;
	}
	return true;
}

// ============================================================================
// DEBUG APIs
// ============================================================================

void vma_print_areas(struct list_head *head)
{
	struct vm_area *area;
	int             i = 0;
	vga_printf("  - VMA areas:\n");
	list_for_each_entry(area, head, vma_node)
	{
		vga_printf("    [%d] vaddr=%p size=%u state=%s\n", i++, (void *)area->start_vaddr,
		           area->size, area->state == VM_AREA_FREE ? "FREE" : "ALLOC");
	}
}
