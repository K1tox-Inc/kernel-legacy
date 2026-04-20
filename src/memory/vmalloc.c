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
	struct vm_area *area = vma_find_by_start(ptr, &vmalloc_areas);
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

	uintptr_t       pd   = get_current_page_directory_phys();
	struct vm_area *area = vma_alloc(&vmalloc_areas, pd, size, PTE_PRESENT_BIT | PTE_RW_BIT, NULL);
	if (!area)
		return NULL;

	return (void *)area->start_vaddr;
}

void vmalloc_init(void) { vma_init_area(&vmalloc_areas, VMALLOC_START, VMALLOC_END); }
