#include <arch/acpi.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/boot_allocator.h>
#include <memory/buddy.h>
#include <memory/page.h>
#include <utils/kmacro.h>

// Defines
static const char *debug_buddy_zone_to_str(zone_type zone);

// Macros

#define PAGE_DATA_IS_MAGIC(page)           ((page)->private_data == PAGE_MAGIC)
#define WHO_IS_MY_BUDDY(addr, order, base) ((((addr) - (base)) ^ ORDER_TO_BYTES(order)) + (base))

// STRUCT

// VARIABLES GLOBALES

static buddy_allocator_t buddy[MAX_ZONE];

// Internals APIs
static const char *debug_buddy_order_to_string(int order)
{
	switch (order) {
	case 0:
		return "4KiB";
	case 1:
		return "8KiB";
	case 2:
		return "16KiB";
	case 3:
		return "32KiB";
	case 4:
		return "64KiB";
	case 5:
		return "128KiB";
	case 6:
		return "256KiB";
	case 7:
		return "512KiB";
	case 8:
		return "1MiB";
	case 9:
		return "2MiB";
	case 10:
		return "4MiB";
	default:
		return "Unknown order";
	}
}

static inline bool order_is_valid(int order) { return order >= 0 && order <= MAX_ORDER; }

static inline uintptr_t *page_node_to_phys(struct list_head *page_node)
{
	return (uintptr_t *)page_to_phys(container_of(page_node, page_t, node));
}

static inline struct list_head *order_to_free_list(size_t order, zone_type zone)
{
	return &buddy[zone].areas[order].free_list[MIGRATE_MOVABLE];
}

static inline size_t order_to_nrFree(size_t order, zone_type zone)
{
	return buddy[zone].areas[order].nr_free;
}

static void buddy_list_add_head(struct list_head *new_node, struct list_head *head)
{
	struct list_head *old_next = head->next;

	new_node->next = old_next;
	new_node->prev = head;
	old_next->prev = new_node;
	head->next     = new_node;
}

static size_t size_to_order(size_t size)
{
	size_t total_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	for (int order = 0; order <= MAX_ORDER; order++) {
		if (total_pages <= (size_t)PAGE_BY_ORDER(order))
			return order;
	}
	return BAD_ORDER;
}

static void pop_node(struct list_head *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next       = NULL;
	node->prev       = NULL;
}

static uintptr_t *pop_first_block(size_t order, zone_type zone)
{
	struct list_head *head        = order_to_free_list(order, zone);
	struct list_head *first_block = head->next;
	if (first_block == head)
		return NULL;

	pop_node(first_block);
	buddy[zone].areas[order].nr_free--;
	return page_node_to_phys(first_block);
}

static void set_block_metadata(page_t *first_page, size_t block_order, uint32_t state_flags)
{
	size_t pages_in_block    = PAGE_BY_ORDER(block_order);
	first_page->private_data = block_order;

	for (size_t i = 0; i < pages_in_block; i++) {
		PAGE_SET_STATE(first_page + i, state_flags);
		if (i > 0) {
			(first_page + i)->private_data = PAGE_MAGIC;
		}
	}
}

static uintptr_t *split_block_to_order(size_t order_needed, size_t cur_order, uintptr_t *ptr,
                                       zone_type zone)
{
	if (!order_is_valid(order_needed) || !order_is_valid(cur_order))
		kpanic("Error: %s: Invalid Order\n", __func__);
	if (cur_order == order_needed) {
		return (uintptr_t *)ptr;
	}

	cur_order--;
	size_t pages_by_block = PAGE_BY_ORDER(cur_order);

	page_t *first_page      = page_addr_to_page((uintptr_t)ptr);
	page_t *buddy_page_head = first_page + pages_by_block;

	set_block_metadata(buddy_page_head, cur_order, PAGE_STATE_FREE);

	buddy_list_add_head(&buddy_page_head->node, order_to_free_list(cur_order, zone));
	buddy[zone].areas[cur_order].nr_free++;

	return split_block_to_order(order_needed, cur_order, ptr, zone);
}

static inline zone_type page_zone_flags_to_zone_type(uint32_t zone_flags)
{
	switch (zone_flags) {
	case PAGE_ZONE_DMA:
		return DMA_ZONE;
	case PAGE_ZONE_LOWMEM:
		return LOWMEM_ZONE;
	case PAGE_ZONE_HIGHMEM:
		return HIGHMEM_ZONE;
	default:
		return INVALID_ZONE;
	}
}

// Refactor
static uintptr_t get_buddy_base(zone_type zone)
{
	switch (zone) {
	case DMA_ZONE:
		return 0;
	case LOWMEM_ZONE:
		return LOWMEM_START;
	case HIGHMEM_ZONE:
		return HIGHMEM_START;
	default:
		kpanic("get_buddy_base: Invalid zone type %d", zone);
		return 0;
	}
}

// Refactor
static page_t *get_buddy_page(void *block, size_t order, zone_type zone)
{
	uintptr_t buddy_phys_addr = WHO_IS_MY_BUDDY((uintptr_t)block, order, get_buddy_base(zone));
	page_t   *buddy_page      = page_addr_to_page(buddy_phys_addr);

	if (!buddy_page)
		return NULL;
	else if (PAGE_IS_FREE(buddy_page) && (size_t)buddy_page->private_data == order)
		return buddy_page;
	return NULL;
}

// Externals APIs

size_t buddy_print(zone_type zone)
{
	vga_printf("\n--- Buddy Free Blocks in Zone: %s ---\n", debug_buddy_zone_to_str(zone));

	bool   has_blocks       = false;
	size_t total_free_bytes = 0;

	for (int order = 0; order <= MAX_ORDER; order++) {
		size_t nr_free = buddy[zone].areas[order].nr_free;
		if (nr_free > 0) {
			size_t bytes_in_order = nr_free * ORDER_TO_BYTES(order);
			total_free_bytes += bytes_in_order;
			vga_printf("[%s:%u] ", debug_buddy_order_to_string(order), nr_free);
			has_blocks = true;
		}
	}

	if (!has_blocks) {
		vga_printf("(empty)\n");
	} else {
		vga_printf("\nTotal Free in Zone: %u KiB (%u MiB)\n", total_free_bytes / 1024,
		           total_free_bytes / (1024 * 1024));
	}

	return total_free_bytes;
}

// Still useful?
void buddy_print_summary(void)
{
	vga_printf("\n================ Buddy Allocator Summary ================\n");

	size_t total_free_memory = 0;

	for (int zone = 0; zone < MAX_ZONE; zone++) {
		total_free_memory += buddy_print((zone_type)zone);
	}

	vga_printf("\n-------------------------------------------------------\n");
	vga_printf("Total Free Kernel Memory: %u KiB (%u MiB)\n", total_free_memory / 1024,
	           total_free_memory / (1024 * 1024));
	vga_printf("=======================================================\n");
}

size_t buddy_get_var_size(void *var)
{
	page_t *page = page_addr_to_page((uintptr_t)var);
	if (PAGE_DATA_IS_MAGIC(page))
		return 0;
	return PAGE_BY_ORDER(page->private_data) * PAGE_SIZE;
}

uintptr_t *buddy_alloc_pages(size_t size, zone_type zone)
{
	size_t order_needed = size_to_order(size);
	if (!order_is_valid(order_needed))
		return NULL;

	size_t cur_order = order_needed;
	while (cur_order != BAD_ORDER && !order_to_nrFree(cur_order, zone))
		cur_order++;
	if (cur_order == BAD_ORDER)
		return NULL;

	void *block = pop_first_block(cur_order, zone);
	if (block == NULL) {
		kpanic("nr_free mismatch in alloc_pages for order %u", cur_order);
		return NULL;
	}

	void *ret = split_block_to_order(order_needed, cur_order, block, zone);

	page_t *first_page = page_addr_to_page((uintptr_t)ret);
	set_block_metadata(first_page, order_needed, PAGE_STATE_ALLOCATED);
	return (uintptr_t *)ret;
}

// TODO : Compound page
void buddy_free_block(void *ptr)
{
	page_t  *page          = page_addr_to_page((uintptr_t)ptr);
	size_t   block_order   = (size_t)page->private_data;
	uint32_t current_state = PAGE_GET_STATE(page);

	if (current_state == PAGE_STATE_FREE)
		kpanic("Error: %s: Double free detected on address %p\n", __func__, ptr);
	else if (current_state != PAGE_STATE_ALLOCATED && current_state != PAGE_STATE_AVAILABLE)
		kpanic("Error: %s: Attempt to free a page with invalid state (%d) at %p\n", __func__,
		       current_state, ptr);
	else if (!order_is_valid(block_order)) {
		kpanic("Error: buddy_free_block: incorrect block order: %x\n", block_order);
	}

	zone_type zone = page_zone_flags_to_zone_type(PAGE_GET_ZONE(page));
	while (block_order < MAX_ORDER) {
		page_t *buddy_page = get_buddy_page(ptr, block_order, zone);
		if (!buddy_page)
			break;
		pop_node(&buddy_page->node);
		buddy[zone].areas[block_order].nr_free--;
		ptr = (void *)MIN((uintptr_t)ptr, page_to_phys(buddy_page));
		block_order++;
	}

	page_t *final_page = page_addr_to_page((uintptr_t)ptr);

	set_block_metadata(final_page, block_order, PAGE_STATE_FREE);
	buddy_list_add_head(&final_page->node, order_to_free_list(block_order, zone));
	buddy[zone].areas[block_order].nr_free++;
}

void buddy_init(void)
{
	for (size_t zone = 0; zone < MAX_ZONE; zone++) {
		for (int order = 0; order <= MAX_ORDER; order++) {
			struct list_head *head           = &buddy[zone].areas[order].free_list[MIGRATE_MOVABLE];
			head->next                       = head;
			head->prev                       = head;
			buddy[zone].areas[order].nr_free = 0;
		}
	}
	for (uint32_t i = 0; i < total_pages; i++) {
		page_t *page = &page_descriptors[i];

		if (PAGE_GET_STATE(page) == PAGE_STATE_AVAILABLE) {
			page->private_data = 0;
			buddy_free_block((void *)page_to_phys(page));
		}
	}
	boot_allocator_freeze();
}

// DEBUG

static const char *debug_buddy_zone_to_str(zone_type zone)
{
	switch (zone) {
	case 0:
		return "LOWMEM zone";
	case 1:
		return "DMA zone";
	case 2:
		return "HIGHMEM zone";
	default:
		return "Unknown zone";
	}
}

static void debug_buddy_panic(const char *func)
{
	vga_printf("  Panic system here %s()\n", func);
	vga_printf("=========================================\n");
	halt();
}

static void debug_buddy_print_node_info(struct list_head *node)
{
	if (!node) {
		vga_printf("    [Node State] NULL pointer\n");
		return;
	}
	vga_printf("    [Node %p State] | next: %p | prev: %p\n", node, node->next, node->prev);
	if (node->next)
		vga_printf("        -> node->next->prev: %p (should be %p)\n", node->next->prev, node);
}

static void debug_buddy_corrupted_list(size_t order, zone_type zone)
{
	struct list_head *cur;
	size_t            count = 0;
	size_t            size  = order_to_nrFree(order, zone);
	size_t            limit = (size == 0 ? 2000 : size * 4);
	struct list_head *head  = &buddy[zone].areas[order].free_list[MIGRATE_MOVABLE];

	for (cur = head->next; cur != head; cur = cur ? cur->next : NULL) {
		count++;
		if (!cur) {
			vga_printf("\n\n--- KERNEL PANIC: LIST CORRUPTION ---\n");
			vga_printf("  Buddy free_list for order %s from %s is corrupted!\n\n",
			           debug_buddy_order_to_string(order), debug_buddy_zone_to_str(zone));
			vga_printf("  Reason: NULL pointer encountered during traversal (node #%zu)\n", count);
			debug_buddy_print_node_info(cur);
			debug_buddy_panic(__func__);
		}
		if (count > limit) {
			vga_printf("\n\n--- KERNEL PANIC: LIST CORRUPTION ---\n");
			vga_printf("  Buddy free_list for order %s from %s is corrupted!\n\n",
			           debug_buddy_order_to_string(order), debug_buddy_zone_to_str(zone));
			vga_printf("  Reason: Infinite loop or corruption detected (node limit exceeded)\n");
			vga_printf("  Info: limit(%u) head(%p) cur(%p) cur->next(%p)\n", limit, head, cur,
			           cur->next);
			debug_buddy_print_node_info(cur);
			debug_buddy_panic(__func__);
		}
	}
}

static void print_buddy_free_list(size_t order, zone_type zone)
{
	struct list_head *head     = &buddy[zone].areas[order].free_list[MIGRATE_MOVABLE];
	struct list_head *cur      = head->next;
	int               node_num = 0;

	debug_buddy_corrupted_list(order, zone);

	vga_printf("=== Free list [order %s] ===\n", debug_buddy_order_to_string(order));
	vga_printf("Head : %p\n Size : %d\n", head, order_to_nrFree(order, zone));
	while (cur != head) {
		vga_printf("  Node %d : %p | next = %p | prev = %p\n", node_num, cur, cur->next, cur->prev);
		cur = cur->next;
		node_num++;
	}
}

// refactor
static void debug_buddy_check_lost_pages(void)
{
	size_t    lost        = 0;
	size_t    total_buddy = 0;
	size_t    free_count  = boot_allocator_get_region_count(FREE_MEMORY);
	region_t *free_reg    = boot_allocator_get_region(FREE_MEMORY);

	for (uint32_t i = 0; i < total_pages; i++) {
		if (PAGE_IS_FREE(&page_descriptors[i])) {
			uintptr_t addr     = i * PAGE_SIZE;
			bool      in_buddy = false;
			for (size_t j = 0; j < free_count; j++) {
				if (addr >= free_reg[j].start && addr < free_reg[j].end) {
					in_buddy = true;
					break;
				}
			}
			if (!in_buddy) {
				vga_printf("Lost page: 0x%x\n", addr);
				lost++;
			} else {
				total_buddy++;
			}
		}
	}

	if (lost > 0) {
		vga_printf("\n\n--- KERNEL PANIC: PAGES Missing ---\n");
		vga_printf("Total lost pages: %u\n", lost);
		debug_buddy_panic(__func__);
	}
}

static void debug_buddy_alloc_still_free(size_t order, void *phys, zone_type zone)
{
	struct list_head *head = order_to_free_list(order, zone);
	for (struct list_head *cur = head->next; cur != head; cur = cur->next) {
		if ((uintptr_t)page_node_to_phys(cur) == (uintptr_t)phys) {
			vga_printf("Error: Test failed in %s: Allocated block (%p) is STILL present in free "
			           "list for order %s!\n",
			           __func__, phys, debug_buddy_order_to_string(order));
			debug_buddy_panic(__func__);
		}
	}
}

static inline uintptr_t *alloc_block_with_order(size_t order, zone_type zone)
{
	return buddy_alloc_pages(PAGE_BY_ORDER(order) * PAGE_SIZE, zone);
}

static void buddy_drain_lower_orders(zone_type zone)
{
	for (int order = 0; order < MAX_ORDER; order++) {
		while (buddy[zone].areas[order].nr_free > 0) {
			uintptr_t *blk = alloc_block_with_order(order, zone);
			if (blk == NULL) {
				vga_printf("Error: Unexpected NULL while draining order %d\n", order);
				debug_buddy_panic(__func__);
			}
		}

		if (order_to_nrFree(order, zone) != 0) {
			vga_printf("Error: After draining, order %d still has %u free blocks\n", order,
			           order_to_nrFree(order, zone));
			debug_buddy_panic(__func__);
		}
	}
}

static void debug_buddy_simple_alloc(zone_type zone)
{
	for (int i = MAX_ORDER; i >= 0; i--) {
		if (order_to_nrFree(i, zone) > 0) {
			uintptr_t *ret = alloc_block_with_order(i, zone);
			debug_buddy_alloc_still_free(i, ret, zone);
		}
	}
}

static void debug_buddy_check_all_list_corrumption(zone_type zone)
{
	for (size_t order = 0; order <= MAX_ORDER; order++)
		debug_buddy_corrupted_list(order, zone);
}

static void debug_buddy_split_block(zone_type zone)
{
	buddy_drain_lower_orders(zone);
	for (int order = MAX_ORDER - 1; order >= 0; order--) {
		uintptr_t *block = buddy_alloc_pages(ORDER_TO_BYTES(order), zone);
		if (block == NULL) {
			vga_printf("No more block for testing split, order upper than %s on %s [OK]\n",
			           debug_buddy_order_to_string(order), debug_buddy_zone_to_str(zone));
			return;
		}
		debug_buddy_alloc_still_free(order, block, zone);
		for (int i = order; i < MAX_ORDER; i++) {
			if (order_to_nrFree(i, zone) != 1) {
				vga_printf("Error: order %d should have exactly 1 free block, found %u\n", i,
				           order_to_nrFree(i, zone));
				print_buddy_free_list(i, zone);
				debug_buddy_panic(__func__);
			}
		}
		debug_buddy_check_all_list_corrumption(zone);
		debug_buddy_simple_alloc(zone);
		buddy_drain_lower_orders(zone);
	}
}

static void debug_buddy_free_block(zone_type zone)
{
	buddy_drain_lower_orders(zone);

	for (size_t order = 0; order <= MAX_ORDER; order++) {
		uintptr_t *block = buddy_alloc_pages(ORDER_TO_BYTES(order), zone);
		debug_buddy_alloc_still_free(order, block, zone);

		buddy_free_block(block);

		for (size_t i = order; i < MAX_ORDER; i++) {
			if (order_to_nrFree(i, zone) != 0) {
				vga_printf("Error: order %u should have exactly 0 free block, found %u\n", i,
				           order_to_nrFree(i, zone));
				print_buddy_free_list(i, zone);
				debug_buddy_panic(__func__);
			}
		}
	}
}

// Still useful?
void debug_buddy(void)
{
	for (size_t zone = 0; zone < MAX_ZONE; zone++) {
		// 	for (int order = 0; order <= MAX_ORDER; order++) {
		// 		debug_buddy_corrupted_list(order, zone);
		// 		vga_printf("OK %s : %s\n", debug_buddy_zone_to_str(zone),
		// debug_buddy_order_to_string(order));
		// 	}
		// }
		// if (DMA_ZONE == zone)
		// 	continue ;
		vga_printf("Debug on %s\n", debug_buddy_zone_to_str(zone));

		// Missing Pages
		debug_buddy_check_lost_pages();
		vga_printf("[OK] No pages missing\n");

		// List Corrumption
		debug_buddy_check_all_list_corrumption(zone);
		vga_printf("[OK] Buddy free_list is healthy\n");

		// Simple alloc
		debug_buddy_simple_alloc(zone);
		debug_buddy_check_all_list_corrumption(zone);
		vga_printf("[OK] First test for simple allocation\n");

		// Free Blocks
		debug_buddy_free_block(zone);
		vga_printf("[OK] Free Blocks\n");

		// Split allocation
		debug_buddy_split_block(zone);
		vga_printf("[OK] Split Blocks\n");

		// Free Invalid Pointer
		// buddy_drain_lower_orders();
		// buddy_free_block(0x01000);

		// Alloc Invalid Size
		buddy_drain_lower_orders(zone);
		vga_printf("-----\n");
	}
}
