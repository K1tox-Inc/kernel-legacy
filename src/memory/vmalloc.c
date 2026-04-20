// ============================================================================
// INCLUDES
// ============================================================================

#include <kernel/panic.h>
#include <libk.h>
#include <list.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/page.h>
#include <memory/vma.h>
#include <memory/vmm.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

#define MAX_VMALLOC_SIZE ((MiB_SIZE * 128) - 8192)
#define MIN_VMALLOC_SIZE PAGE_SIZE

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

static struct list_head vmalloc_areas;

// ============================================================================
// EXTERNAL APIs
// ============================================================================

static inline size_t vsize(void *ptr) { return (vma_size(ptr, &vmalloc_areas)); }

void vfree(void *ptr)
{
	if (!ptr)
		return;
	struct vm_area *area = vma_find_area(ptr, &vmalloc_areas);
	if (!area)
		return;

	uintptr_t current_page_dir = get_current_page_directory_phys();

	for (size_t i = 0; i < area->nr_pages; i++) {
		vmm_unmap_page(current_page_dir, area->start_vaddr + (i * PAGE_SIZE));
		buddy_free_block((void *)area->pages[i]);
	}

	kfree(area->pages);
	area->nr_pages = 0;
	area->state    = VM_AREA_FREE;
	vma_merge_area(&vmalloc_areas, area);
}

void *vmalloc(size_t size)
{
	if (size > MAX_VMALLOC_SIZE || size < MIN_VMALLOC_SIZE)
		return NULL;

	struct vm_area *area = vma_first_fit_alloc(&vmalloc_areas, size);
	if (!area)
		return NULL;

	struct vm_area *allocated_area = vma_split_area(area, size);
	if (!allocated_area)
		return NULL;

	size_t number_of_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	allocated_area->pages  = kmalloc(sizeof(uintptr_t) * number_of_pages, GFP_KERNEL);
	if (!allocated_area->pages)
		goto free_allocated_area;
	allocated_area->nr_pages = number_of_pages;

	size_t num_of_alloc = 0;
	for (size_t i = 0; i < number_of_pages; i++, num_of_alloc++) {
		void *page = buddy_alloc_pages(PAGE_SIZE, HIGHMEM_ZONE);
		if (!page)
			goto free_allocated_pages;
		allocated_area->pages[i] = (uintptr_t)page;
	}
	if (num_of_alloc != number_of_pages)
		goto free_allocated_pages;

	uintptr_t current_page_dir = get_current_page_directory_phys();
	for (size_t i = 0; i < number_of_pages; i++) {
		uintptr_t paddr = allocated_area->pages[i];
		uintptr_t vaddr = allocated_area->start_vaddr + (i * PAGE_SIZE);
		// Can fail only if buddy is out of memory, im lazy to implement the unmap label for this
		// case
		if (!vmm_map_page(current_page_dir, vaddr, paddr, PTE_PRESENT_BIT | PTE_RW_BIT))
			kpanic("vmalloc: vmm_map_page failed!");
	}

	return (void *)allocated_area->start_vaddr;

free_allocated_pages:
	for (size_t i = 0; i < num_of_alloc; i++)
		buddy_free_block((void *)allocated_area->pages[i]);
	kfree(allocated_area->pages);
free_allocated_area:
	area->state = VM_AREA_FREE;
	vma_merge_area(&vmalloc_areas, area);
	kfree(allocated_area);

	return NULL;
}

void vmalloc_init(void)
{
	struct vm_area *initial_hole = kmalloc(sizeof(struct vm_area), GFP_KERNEL);
	if (!initial_hole)
		kpanic("vmalloc_init failed!");

	*initial_hole = (struct vm_area){
	    .state       = VM_AREA_FREE,
	    .start_vaddr = VMALLOC_START,
	    .size        = VMALLOC_END - VMALLOC_START,
	    .nr_pages    = 0,
	    .pages       = NULL,
	};

	INIT_SENTINEL(&vmalloc_areas);
	list_add_head(&initial_hole->vma_node, &vmalloc_areas);
}
