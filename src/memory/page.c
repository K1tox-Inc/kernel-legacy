#include <arch/acpi.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <memory/boot_allocator.h>
#include <memory/memory.h>
#include <memory/page.h>

typedef void (*pages_foreach_fn)(page_t *page, void *data);

uint32_t reserved_count   = 0;
uint32_t page_free_count  = 0;
page_t  *page_descriptors = NULL;

static bool page_addr_is_in_reg(region_t *free_regions_in_zone, uint32_t region_count_in_zone,
                                uintptr_t addr)
{
	for (uint32_t i = 0; i < region_count_in_zone; i++) {
		region_t *reg = &free_regions_in_zone[i];
		if (addr >= reg->start && addr < reg->end) {
			return true;
		}
	}
	return false;
}

static uint32_t page_get_state_flag(uintptr_t addr_start)
{
	for (int zone_idx = 0; zone_idx < MAX_ZONE; zone_idx++) {

		uint32_t  free_count = boot_allocator_get_free_zones_count(zone_idx);
		region_t *free_reg   = boot_allocator_get_free_zone(zone_idx);
		uint32_t  res_count  = boot_allocator_get_res_zones_count(zone_idx);
		region_t *res_reg    = boot_allocator_get_res_zone(zone_idx);

		if (page_addr_is_in_reg(free_reg, free_count, addr_start))
			return (PAGE_STATE_AVAILABLE);
		else if (page_addr_is_in_reg(res_reg, res_count, addr_start))
			return (PAGE_STATE_RESERVED);
	}
	return PAGE_STATE_UNUSABLE;
}

static uint32_t page_get_zone_flag(uintptr_t addr_start)
{
	if (addr_start >= HIGHMEM_START)
		return PAGE_ZONE_HIGHMEM;
	else if (addr_start >= LOWMEM_START)
		return PAGE_ZONE_LOWMEM;
	return PAGE_ZONE_DMA;
}

static inline uint32_t page_get_appropriate_flag(uintptr_t addr_start)
{
	return page_get_state_flag(addr_start) | page_get_zone_flag(addr_start);
}

static void count_free_pages(page_t *page, void *counter)
{
	if (PAGE_IS_FREE(page))
		(*(uint32_t *)counter)++;
}

static void count_reserved_pages(page_t *page, void *counter)
{
	if (!PAGE_IS_FREE(page))
		(*(uint32_t *)counter)++;
}

static inline page_t *last_page() { return &page_descriptors[total_pages - 1]; }

static inline page_t *first_page() { return &page_descriptors[0]; }

static void page_descriptor_foreach(pages_foreach_fn handler, void *data)
{
	for (uint32_t i = 0; i < total_pages; i++)
		handler(&page_descriptors[i], data);
}

// External Apis

uint32_t page_to_index(page_t *page) { return page - page_descriptors; }

uintptr_t page_to_phys(page_t *page)
{
	uint32_t idx = page_to_index(page);
	return idx * PAGE_SIZE;
}

page_t *page_index_to_page(uint32_t idx)
{
	if (idx >= total_pages)
		return NULL;
	return &page_descriptors[idx];
}

page_t *page_addr_to_page(uintptr_t addr)
{
	return &page_descriptors[ALIGN_DOWN(addr, PAGE_SIZE) / PAGE_SIZE];
}

bool page_addr_is_same_page(uintptr_t addr1, uintptr_t addr2)
{
	return (addr1 / PAGE_SIZE) == (addr2 / PAGE_SIZE);
}

void page_print_info(page_t *page)
{
	if (!page) {
		vga_printf("page_print_info: Page pointer is NULL\n");
		return;
	}

	uint32_t  index = page_to_index(page);
	uintptr_t addr  = page_to_phys(page);

	vga_printf("Page Info for PFN %u (Phys Addr: %p):\n", index, addr);
	vga_printf("  - Raw Flags: 0x%x\n", page->flags);

	vga_printf("  - State: ");
	switch (PAGE_GET_STATE(page)) {
	case PAGE_STATE_UNUSABLE:
		vga_printf("UNUSABLE (Memory hole or MMIO)\n");
		break;
	case PAGE_STATE_RESERVED:
		vga_printf("RESERVED (By boot allocator)\n");
		break;
	case PAGE_STATE_AVAILABLE:
		vga_printf("AVAILABLE (Ready for Buddy System)\n");
		break;
	case PAGE_STATE_FREE:
		vga_printf("FREE (In Buddy free list)\n");
		break;
	case PAGE_STATE_ALLOCATED:
		vga_printf("ALLOCATED (By Buddy)\n");
		break;
	case PAGE_STATE_SLAB:
		vga_printf("SLAB (Allocated for Slab)\n");
		break;
	default:
		vga_printf("UNKNOWN STATE!\n");
		break;
	}

	vga_printf("  - Zone:  ");
	switch (PAGE_GET_ZONE(page)) {
	case PAGE_ZONE_DMA:
		vga_printf("DMA (< 16MB)\n");
		break;
	case PAGE_ZONE_LOWMEM:
		vga_printf("LOWMEM (16MB - 896MB)\n");
		break;
	case PAGE_ZONE_HIGHMEM:
		vga_printf("HIGHMEM (> 896MB)\n");
		break;
	default:
		vga_printf("N/A\n");
		break;
	}

	if (PAGE_IS_FREE(page) || PAGE_IS_ALLOCATED(page)) {
		vga_printf("  - Buddy Order: %u\n", page->private_data);
	} else if (page->private_data != 0) {
		vga_printf("  - Private Data: 0x%x\n", page->private_data);
	}
}

uint32_t page_get_updated_reserved_count(void)
{
	reserved_count = 0;
	page_descriptor_foreach(count_reserved_pages, &reserved_count);
	return reserved_count;
}

uint32_t page_get_updated_page_free_count(void)
{
	page_free_count = 0;
	page_descriptor_foreach(count_free_pages, &page_free_count);
	return page_free_count;
}

page_t *page_addr_to_usable(uintptr_t addr, bool direction)
{
	int     factor = (direction == NEXT ? 1 : -1);
	page_t *ret    = page_addr_to_page(addr);

	while (ret >= first_page() && ret <= last_page()) {
		if (PAGE_IS_FREE(ret))
			return ret;

		ret += factor;
	}
	return NULL;
}

uint32_t page_get_page_free_count(void) { return page_free_count; }

uint32_t page_get_reserved_count(void) { return reserved_count; }

void page_descriptor_init(void)
{
	uintptr_t p_addr = (uintptr_t)boot_alloc(MAX_PAGES * sizeof(page_t), LOWMEM_ZONE, TO_KEEP);
	if (!p_addr) {
		kpanic("Failed to allocate page descriptors!");
	}
	page_descriptors = (page_t *)PHYS_TO_VIRT_LINEAR(p_addr);

	for (uint32_t i = 0; i < MAX_PAGES; i++) {
		page_descriptors[i].flags        = page_get_appropriate_flag(i * PAGE_SIZE);
		page_descriptors[i].private_data = PAGE_MAGIC;
	}
}
