// ============================================================================
// INCLUDES
// ============================================================================

#include <kernel/panic.h>
#include <libk.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/page.h>
#include <memory/vmm.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

#define MAX_VMALLOC_SIZE ((MiB_SIZE * 128) - 8192)
#define MIN_VMALLOC_SIZE PAGE_SIZE

// Macros

#define INIT_SENTINEL(name, ptr)                                                                   \
	do {                                                                                           \
		struct list_head *name = ptr;                                                              \
		name->next             = name;                                                             \
		name->prev             = name;                                                             \
	} while (0)

// ============================================================================
// STRUCT
// ============================================================================

// Enums

// Structures

typedef enum { VM_AREA_FREE, VM_AREA_ALLOCATED } vm_area_state_t;

typedef struct vm_area {
	struct list_head list;
	vm_area_state_t  state;
	uintptr_t        start_vaddr;
	size_t           size;

	// Only initialized when state is VM_AREA_ALLOCATED otherwise set as NULL
	size_t     nr_pages;
	uintptr_t *pages;

} vm_area_t;

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

static struct list_head vmalloc_areas;

static const zone_type vmalloc_zonelist[] = {HIGHMEM_ZONE, LOWMEM_ZONE, INVALID_ZONE};

// ============================================================================
// INTERNAL APIs
// ============================================================================

static void list_add_head(struct list_head *new_node, struct list_head *head)
{
	struct list_head *old_next = head->next;

	new_node->next = old_next;
	new_node->prev = head;
	old_next->prev = new_node;
	head->next     = new_node;
}

static void list_insert(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next  = next;
	new->prev  = prev;
	prev->next = new;
}

static void pop_node(struct list_head *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next       = NULL;
	node->prev       = NULL;
}

static vm_area_t *first_fit_alloc(size_t size)
{
	struct list_head *start = vmalloc_areas.next;
	while (start != &vmalloc_areas) {
		vm_area_t *area = container_of(start, vm_area_t, list);
		if (area->size >= size && area->state == VM_AREA_FREE)
			return area;
		start = start->next;
	}
	return NULL;
}

static vm_area_t *split_area(vm_area_t *area, size_t size)
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

	vm_area_t *new_area = kmalloc(sizeof(vm_area_t), GFP_KERNEL);
	if (!new_area)
		return NULL;

	new_area->state    = VM_AREA_ALLOCATED;
	new_area->size     = needed_size;
	new_area->nr_pages = num_of_pages;

	new_area->start_vaddr = area->start_vaddr;
	area->start_vaddr += needed_size;
	area->size -= needed_size;

	list_insert(&new_area->list, area->list.prev, &area->list);

	return new_area;
}

static void merge_area(vm_area_t *area)
{
	struct list_head *next_node = area->list.next;

	if (next_node != &vmalloc_areas) {
		vm_area_t *next_area = container_of(next_node, vm_area_t, list);

		if (next_area->state == VM_AREA_FREE &&
		    area->start_vaddr + area->size == next_area->start_vaddr) {
			area->size += next_area->size;
			pop_node(&next_area->list);
			kfree(next_area);
		}
	}

	struct list_head *prev_node = area->list.prev;

	if (prev_node != &vmalloc_areas) {
		vm_area_t *prev_area = container_of(prev_node, vm_area_t, list);

		if (prev_area->state == VM_AREA_FREE &&
		    prev_area->start_vaddr + prev_area->size == area->start_vaddr) {
			prev_area->size += area->size;
			pop_node(&area->list);
			kfree(area);
		}
	}
}

// ============================================================================
// EXTERNAL APIs
// ============================================================================

size_t vsize(void *ptr)
{
	if (!ptr)
		return 0;
	for (struct list_head *head = vmalloc_areas.next; head != &vmalloc_areas; head = head->next) {
		vm_area_t *area = container_of(head, vm_area_t, list);
		if (area->start_vaddr == (uintptr_t)ptr)
			return area->size;
	}
	return 0;
}

/*
 * need to be free :
 * 1 - area if merged
 * 2 - pages and page one by one
 * 3 - page dir entry if a full entry is unmaped
 *
 * fun needed :
 * one to merge
 * free all
 */
void vfree(void *ptr)
{
	if (!ptr)
		return;
	vm_area_t *area = NULL;
	for (struct list_head *head = vmalloc_areas.next; head != &vmalloc_areas; head = head->next) {
		vm_area_t *tmp_area = container_of(head, vm_area_t, list);
		if (tmp_area->start_vaddr == (uintptr_t)ptr) {
			area = tmp_area;
			break;
		}
	}
	if (!area)
		return;
	area->state = VM_AREA_FREE;
	// step 1 free pages one by one and unmap finish by free pages
	uintptr_t current_page_dir = get_current_page_directory_phys();
	for (size_t i = 0; i < area->nr_pages; i++) {
		uintptr_t vaddr = area->start_vaddr + (i * PAGE_SIZE);
		vmm_unmap_page(current_page_dir, vaddr);
		buddy_free_block((void *)area->pages[i]);
	}
	area->nr_pages = 0;
	kfree(area->pages);
	// step 2 merge with neighbors + free everythings
	merge_area(area);
}

void *vmalloc(size_t size)
{
	if (size > MAX_VMALLOC_SIZE || size < MIN_VMALLOC_SIZE)
		return NULL;

	vm_area_t *area = first_fit_alloc(size);
	if (!area)
		return NULL;
	vm_area_t *allocated_area = split_area(area, size);
	if (!allocated_area)
		return NULL;

	size_t number_of_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	// firstStep get all page + fallback system convert page in uintptr to use it as int
	allocated_area->pages = kmalloc(sizeof(uintptr_t) * number_of_pages, GFP_KERNEL);
	if (!allocated_area->pages) {
		merge_area(allocated_area);
		kfree(allocated_area);
		return NULL;
	}
	allocated_area->nr_pages = number_of_pages;

	size_t num_of_alloc = 0;
	for (size_t i = 0; vmalloc_zonelist[i] != INVALID_ZONE; i++) {
		if (num_of_alloc == number_of_pages)
			break;
		int current_zone = vmalloc_zonelist[i];
		while (num_of_alloc < number_of_pages) {
			void *page = buddy_alloc_pages(PAGE_SIZE, current_zone);
			if (!page)
				break;
			allocated_area->pages[num_of_alloc] = (uintptr_t)page;
			num_of_alloc++;
		}
	}
	if (num_of_alloc != number_of_pages) {
		for (size_t i = 0; i < num_of_alloc; i++)
			buddy_free_block((void *)allocated_area->pages[i]);
		allocated_area->nr_pages = 0;
		kfree(allocated_area->pages);
		merge_area(allocated_area);
		kfree(allocated_area);
		return NULL;
	}
	// secondStep map all page in vmalloc are
	uintptr_t current_page_dir = get_current_page_directory_phys();
	for (size_t i = 0; i < number_of_pages; i++) {
		uintptr_t paddr = allocated_area->pages[i];
		uintptr_t vaddr = allocated_area->start_vaddr + (i * PAGE_SIZE);
		if (vmm_map_page(current_page_dir, vaddr, paddr, PTE_PRESENT_BIT | PTE_RW_BIT) == false)
			kpanic("vmalloc: vmm_map_page failed!");
	}
	return (void *)allocated_area->start_vaddr;
}

void vmalloc_init(void)
{

	vm_area_t *initial_hole = kmalloc(sizeof(vm_area_t), GFP_KERNEL);
	if (!initial_hole)
		kpanic("vmalloc_init failed!");

	initial_hole->state       = VM_AREA_FREE;
	initial_hole->start_vaddr = VMALLOC_START;
	initial_hole->size        = VMALLOC_END - VMALLOC_START;
	initial_hole->nr_pages    = 0;
	initial_hole->pages       = NULL;

	INIT_SENTINEL(sentinel, &vmalloc_areas);
	list_add_head(&initial_hole->list, &vmalloc_areas);
}
