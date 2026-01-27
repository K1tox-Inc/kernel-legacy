// ============================================================================
// INCLUDES
// ============================================================================

#include <kernel/panic.h>
#include <libk.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/page.h>
#include <memory/slab.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

// Macros

// ============================================================================
// STRUCT
// ============================================================================

// Enums

// Structures

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// INTERNAL APIs
// ============================================================================

void *get_allocation(zone_type zone, size_t size)
{
	void *ret = NULL;

	if (size > MAX_SLAB_SIZE) {
		uintptr_t phys_addr = (uintptr_t)buddy_alloc_pages(size, zone);
		if (phys_addr != 0)
			ret = PHYS_TO_VIRT_LINEAR(phys_addr);
	} else
		ret = slab_alloc(size, zone);
	return ret;
}

void *try_alloc_with_reclaim(zone_type zone, size_t size)
{
	void *alloc = get_allocation(zone, size);
	if (alloc == NULL) {
		slab_shrink_caches(zone);
		return get_allocation(zone, size);
	}
	return alloc;
}

// ============================================================================
// EXTERNAL APIs
// ============================================================================

size_t ksize(void *ptr)
{
	if (!ptr)
		return 0;

	uintptr_t phys_addr  = VIRT_TO_PHYS_LINEAR(ptr);
	page_t   *page       = page_addr_to_page(phys_addr);
	size_t    page_state = PAGE_GET_STATE(page);

	if (page_state == PAGE_STATE_SLAB) {
		slab_t *slab = (slab_t *)page->private_data;
		return slab->parent_cache->object_size;
	} else if (page_state == PAGE_STATE_ALLOCATED) {
		if (page->private_data == PAGE_MAGIC)
			return 0;
		else
			return (ORDER_TO_BYTES(page->private_data));
	}
	return 0;
}

void kfree(void *ptr)
{
	if (!ptr)
		return;
	uintptr_t phys_addr  = VIRT_TO_PHYS_LINEAR(ptr);
	page_t   *page       = page_addr_to_page(phys_addr);
	size_t    page_state = PAGE_GET_STATE(page);
	/* TODO: If an use after free of double free is done slab list become corrupt
	 * For performance is better to do not handle the problem,
	 * and give the responsability to the devloppeur
	 * For debugging tricks like red_zone/coloring/poissoning can be implemented
	 */
	if (page_state == PAGE_STATE_SLAB)
		slab_free(ptr);
	else if (page_state == PAGE_STATE_ALLOCATED)
		buddy_free_block((void *)phys_addr);
}

// TODO: implement ATOMIC gfp flags handling when SMP or multi process is ok
void *kmalloc(size_t size, gfp_t flags)
{

	void     *ret  = NULL;
	zone_type zone = LOWMEM_ZONE;
	if (FLAG_IS_SET(flags, __GFP_DMA))
		zone = DMA_ZONE;

	if (size == 0)
		return NULL;
	else if (size > MAX_KMALLOC_SIZE)
		return NULL;

	ret = try_alloc_with_reclaim(zone, size);

	if (ret != NULL && FLAG_IS_SET(flags, __GFP_ZERO))
		ft_bzero(ret, size);
	return ret;
}
