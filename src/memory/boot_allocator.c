#include <arch/acpi.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/boot_allocator.h>
#include <memory/memory.h>
#include <types.h>
#include <utils/kmacro.h>

#include <arch/x86.h>

// Header

static void     boot_allocator_sort_regions(region_t *reg, uint32_t count);
static uint32_t boot_allocator_merge_contiguous_regions(region_t *reg, uint32_t count);

// Defines

#define END               0x00000000
#define REGION_TYPE_COUNT 3
#define MAX_REGIONS       256

// Macros

#define BOOT_ALLOCATOR_SORT_AND_MERGE(regions, count)                                              \
	do {                                                                                           \
		boot_allocator_sort_regions((regions), (count));                                           \
		(count) = boot_allocator_merge_contiguous_regions((regions), (count));                     \
	} while (0)

// Only in if statement
#define BOOT_ALLOCATOR_IS_REGION_UNMAPPED(start, end)                                              \
	(!(boot_allocator_overlaps((start), (end), RESERVED_MEMORY) ||                                 \
	   boot_allocator_overlaps((start), (end), FREE_MEMORY)))

#define BOOT_ALLOC_FREE_COUNT(alloc)     ((alloc)->count[FREE_MEMORY])
#define BOOT_ALLOC_RESERVED_COUNT(alloc) ((alloc)->count[RESERVED_MEMORY])
#define BOOT_ALLOC_HOLE_COUNT(alloc)     ((alloc)->count[HOLES_MEMORY])

#define BOOT_ALLOC_DMA_COUNT(count)     (count[DMA])
#define BOOT_ALLOC_LOWMEM_COUNT(count)  (count[LOWMEM])
#define BOOT_ALLOC_HIGHMEM_COUNT(count) (count[HIGHMEM])

#define BOOT_ALLOC_FREE_REGIONS(alloc)     ((alloc)->regions[FREE_MEMORY])
#define BOOT_ALLOC_RESERVED_REGIONS(alloc) ((alloc)->regions[RESERVED_MEMORY])
#define BOOT_ALLOC_HOLE_REGIONS(alloc)     ((alloc)->regions[HOLES_MEMORY])

// ============================================================================
// STRUCT
// ============================================================================

// Enums

// Structures

typedef struct {
	uintptr_t p_addr;
	size_t    size;
	bool      free;
} boot_alloc_entry_t;

typedef struct boot_allocator {
	bool               state;
	uint32_t           num_allocations;
	boot_alloc_entry_t allocations[MAX_REGIONS];
	uint32_t           count[REGION_TYPE_COUNT];
	region_t           regions[REGION_TYPE_COUNT][MAX_REGIONS];
} boot_allocator_t;

uint32_t free_count[MAX_ZONE];
region_t free_zones[MAX_ZONE][MAX_REGIONS];
uint32_t res_count[MAX_ZONE];
region_t res_zones[MAX_ZONE][MAX_REGIONS];

// Typedefs

typedef void (*regions_foreach_fn)(region_t *regions);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

extern boot_allocator_t bootmem;
static region_t         all_reg_g[MAX_REGIONS * REGION_TYPE_COUNT];

/*
 * The following APIs are used to store early allocations and to keep a track of every used memory
 * before the memory system is available
 */

// Globals

uint32_t         total_pages = 0;
uint32_t         total_RAM   = 0;
boot_allocator_t bootmem;
extern uint8_t   kernel_start[];
extern uint8_t   kernel_end[];
extern uint8_t   kernel_stack_top[];

// Code

static zone_type boot_alloc_get_addr_zone(uintptr_t addr)
{
	if (addr >= HIGHMEM_START) {
		return HIGHMEM_ZONE;
	} else if (addr >= LOWMEM_START) {
		return LOWMEM_ZONE;
	} else {
		return DMA_ZONE;
	}
}

/*
 * Adds a memory region (reserved or free) to the boot allocator's region tables
 * Panics if the region is invalid or if the maximum number of regions is exceeded
 */

static void boot_allocator_add_region(boot_allocator_t *alloc, uintptr_t start, uintptr_t end,
                                      enum mem_type type)
{
	if (start > end && end != 0) {
		kpanic("boot_alloc: %s: Invalid region start=%p end=%p\n", __func__, start, end);
	} else if (start == end) {
		return;
	} else if (type >= REGION_TYPE_COUNT)
		kpanic("boot_alloc: %s: Unknown memory type\n", __func__);

	if (alloc->count[type] >= MAX_REGIONS) {
		kpanic("boot_alloc: %s: too many %s regions\n", __func__);
	}

	uint32_t index              = alloc->count[type];
	alloc->regions[type][index] = (region_t){start, end};
	alloc->count[type]++;
}
static void boot_allocator_add_to_zones(uint32_t count[MAX_ZONE],
                                        region_t zones[MAX_ZONE][MAX_REGIONS], uint32_t size,
                                        uintptr_t start)
{
	zone_type zone = boot_alloc_get_addr_zone(start);
	if (count[zone] >= MAX_REGIONS) {
		kpanic("No more space in zone %d to add a new region!", zone);
	}
	zones[zone][count[zone]++] = (region_t){start, start + size};
	BOOT_ALLOCATOR_SORT_AND_MERGE(zones[zone], count[zone]);
}

/*
 * Wrapper function for adding a reserved region to the boot allocator
 * Used as a callback/handler for memory map iteration
 */

void boot_allocator_reserved_wrapper(uintptr_t start, uintptr_t end)
{
	boot_allocator_add_region(&bootmem, start, end, RESERVED_MEMORY);
}

static void boot_allocator_sort_regions(region_t *reg, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		region_t current = reg[i];
		uint32_t j       = i + 1;

		while (--j > 0 && reg[j - 1].start > current.start)
			reg[j] = reg[j - 1];
		reg[j] = current;
	}
}

static void boot_allocator_free_handler(uintptr_t start, uintptr_t end)
{
	uintptr_t cur = start;
	for (uint32_t i = 0; i < BOOT_ALLOC_RESERVED_COUNT(&bootmem); i++) {
		region_t res = BOOT_ALLOC_RESERVED_REGIONS(&bootmem)[i];

		if (res.end <= cur || res.start >= end)
			continue;
		if (cur < res.start && !boot_allocator_range_overlaps(cur, res.start, FREE_MEMORY)) {
			boot_allocator_add_region(&bootmem, cur, res.start, FREE_MEMORY);
		}
		if (res.end > cur)
			cur = res.end;
		if (cur >= end)
			return;
	}
	if (cur < end && !boot_allocator_range_overlaps(cur, end, FREE_MEMORY)) {
		boot_allocator_add_region(&bootmem, cur, end, FREE_MEMORY);
	}
}

static uint32_t boot_allocator_merge_contiguous_regions(region_t *reg, uint32_t count)
{
	region_t cur          = *reg;
	uint32_t merged_count = 0;
	region_t merged[MAX_REGIONS];

	if (!count)
		return count;
	ft_bzero(merged, sizeof(region_t) * MAX_REGIONS);
	boot_allocator_sort_regions(reg, count);
	for (uint32_t i = 1; i < count; i++) {
		if (cur.end == reg[i].start) {
			cur.end = reg[i].end;
		} else {
			merged[merged_count++] = cur;
			cur                    = reg[i];
		}
	}
	merged[merged_count++] = cur;
	ft_memcpy(reg, merged, sizeof(region_t) * MAX_REGIONS);
	return merged_count;
}

/*
 * Tools for debug :
 * boot_allocator_print_region_info
 * boot_allocator_for_each_regions
 * boot_allocator_print_inital_layout
 */

static void boot_allocator_print_region_info(region_t *reg)
{
	vga_printf("%p -> %p\n", (void *)reg->start, (void *)reg->end);
}

static void boot_allocator_for_each_regions(regions_foreach_fn handler, region_t *reg,
                                            uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		handler(&reg[i]);
	}
}

static region_t *boot_allocator_get_all_regions(boot_allocator_t *alloc)
{
	uint32_t idx = 0;

	for (uint32_t type = 0; type < REGION_TYPE_COUNT; type++) {
		for (uint32_t i = 0; i < alloc->count[type]; i++) {
			ft_memcpy(&all_reg_g[idx++], &alloc->regions[type][i], sizeof(region_t));
		}
	}
	return all_reg_g;
}

static void boot_allocator_fill_gaps_as_holes(void)
{
	uint32_t total_reg = BOOT_ALLOC_RESERVED_COUNT(&bootmem) + BOOT_ALLOC_FREE_COUNT(&bootmem);

	if (total_reg == 0) {
		vga_printf("No regions to process\n");
		return;
	}

	region_t *all_reg = boot_allocator_get_all_regions(&bootmem);

	BOOT_ALLOCATOR_SORT_AND_MERGE(all_reg, total_reg);

	for (uint32_t i = 0; i < total_reg - 1; i++) {
		uintptr_t gap_start = all_reg[i].end;
		uintptr_t gap_end   = all_reg[i + 1].start;
		if (gap_start < gap_end)
			boot_allocator_add_region(&bootmem, gap_start, gap_end, HOLES_MEMORY);
	}

	BOOT_ALLOCATOR_SORT_AND_MERGE(bootmem.regions[HOLES_MEMORY], bootmem.count[HOLES_MEMORY]);
}

static void boot_allocator_init_total_size(uintptr_t start, uintptr_t end)
{
	if ((uint32_t)end == 0)
		end = 0xffffffff;
	total_RAM += end - start;
}

static uint32_t boot_allocator_get_total_visibale_ram(boot_allocator_t *alloc)
{
	uint32_t total_count = BOOT_ALLOC_FREE_COUNT(alloc) + BOOT_ALLOC_RESERVED_COUNT(alloc) +
	                       BOOT_ALLOC_HOLE_COUNT(alloc);

	if (total_count <= 0) {
		kpanic("Error: %s: this function cannot be called before memory parsing\n", __func__);
	}

	region_t *all_reg = boot_allocator_get_all_regions(alloc);
	BOOT_ALLOCATOR_SORT_AND_MERGE(all_reg, total_count);
	uintptr_t end = all_reg[total_count - 1].end;
	if (end == 0)
		return 0xffffffff;
	return end;
}

static void boot_allocator_init_zones(uint32_t zcount[MAX_ZONE],
                                      region_t zones[MAX_ZONE][MAX_REGIONS], enum mem_type type)
{
	size_t    reg_count = boot_allocator_get_region_count(type);
	region_t *reg       = boot_allocator_get_region(type);

	for (size_t i = 0; i < reg_count; i++) {
		uintptr_t region_start = reg[i].start;
		uintptr_t region_end   = reg[i].end;

		uintptr_t cur_start = region_start;
		while (cur_start < region_end) {
			int       zone     = -1;
			uintptr_t zone_end = 0;

			if (cur_start >= HIGHMEM_START) {
				zone     = HIGHMEM_ZONE;
				zone_end = HIGHMEM_END;
			} else if (cur_start >= LOWMEM_START) {
				zone     = LOWMEM_ZONE;
				zone_end = HIGHMEM_START;
			} else {
				zone     = DMA_ZONE;
				zone_end = LOWMEM_START;
			}

			uintptr_t sub_end = (region_end < zone_end) ? region_end : zone_end;
			if (sub_end > cur_start) {
				zones[zone][zcount[zone]++] = (region_t){cur_start, sub_end};
			}
			cur_start = sub_end;
		}
	}
}

// External APis

region_t *boot_allocator_get_region(enum mem_type type) { return bootmem.regions[type]; }

uint32_t boot_allocator_get_region_count(enum mem_type type) { return bootmem.count[type]; }

region_t *boot_allocator_get_free_zone(int type) { return free_zones[type]; }

uint32_t boot_allocator_get_free_zones_count(int type) { return free_count[type]; }

region_t *boot_allocator_get_res_zone(int type) { return res_zones[type]; }

uint32_t boot_allocator_get_res_zones_count(int type) { return res_count[type]; }

void boot_allocator_freeze(void) { bootmem.state = FROZEN; }

/*
 * Checks if a given address range overlaps with any region of the specified memory type
 */

bool boot_allocator_range_overlaps(uintptr_t start, uintptr_t end, enum mem_type type)
{
	uint32_t  count   = bootmem.count[type];
	region_t *regions = bootmem.regions[type];

	for (uint32_t i = 0; i < count; i++) {
		if (!(end <= regions[i].start || start >= regions[i].end)) {
			return true;
		}
	}
	return false;
}

void boot_allocator_print_inital_layout(void)
{
	vga_printf("----------Boot Allocator Printer----------\n");
	vga_printf("Reserved Areas : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info,
	                                BOOT_ALLOC_RESERVED_REGIONS(&bootmem),
	                                BOOT_ALLOC_RESERVED_COUNT(&bootmem));
	vga_printf("----------\n");
	vga_printf("Free Areas : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info,
	                                BOOT_ALLOC_FREE_REGIONS(&bootmem),
	                                BOOT_ALLOC_FREE_COUNT(&bootmem));
	vga_printf("----------\n");
	vga_printf("Holes Areas : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info,
	                                BOOT_ALLOC_HOLE_REGIONS(&bootmem),
	                                BOOT_ALLOC_HOLE_COUNT(&bootmem));
	vga_printf("------------------------------------------\n");
}

void boot_allocator_free_zones_printer(void)
{
	vga_printf("----------Boot Allocator Free Zones Printer----------\n");
	vga_printf("Dma Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, free_zones[DMA_ZONE],
	                                free_count[DMA_ZONE]);
	vga_printf("----------\n");
	vga_printf("Lowmem Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, free_zones[LOWMEM_ZONE],
	                                free_count[LOWMEM_ZONE]);
	vga_printf("----------\n");
	vga_printf("Highmem Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, free_zones[HIGHMEM_ZONE],
	                                free_count[HIGHMEM_ZONE]);
	vga_printf("------------------------------------------\n");
}

void boot_allocator_print_allocations(void)
{
	boot_allocator_t *alloc = &bootmem;
	vga_printf("---------- Boot Allocator Allocations Log ----------\n");

	if (alloc->num_allocations == 0) {
		vga_printf("  No allocations have been made yet.\n");
	} else {
		vga_printf("Index\tP. Addr\t\tSize (B)\tSize (KiB)\tFreeable\n");
		vga_printf("----------------------------------------------------------\n");

		for (uint32_t i = 0; i < alloc->num_allocations; i++) {
			boot_alloc_entry_t *entry = &alloc->allocations[i];

			vga_printf("%u\t", i);
			vga_printf("%p\t\t", (void *)entry->p_addr);
			vga_printf("%u\t\t", entry->size);
			vga_printf("%u\t\t", entry->size / 1024);
			if (entry->free) {
				vga_printf("Yes\n");
			} else {
				vga_printf("No\n");
			}
		}
	}
	vga_printf("----------------------------------------------------------\n");
}

void boot_allocator_res_zones_printer(void)
{
	vga_printf("----------Boot Allocator Reserved Zones Printer----------\n");
	vga_printf("Dma Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, res_zones[DMA_ZONE],
	                                res_count[DMA_ZONE]);
	vga_printf("----------\n");
	vga_printf("Lowmem Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, res_zones[LOWMEM_ZONE],
	                                res_count[LOWMEM_ZONE]);
	vga_printf("----------\n");
	vga_printf("Highmem Zone : \n");
	boot_allocator_for_each_regions(boot_allocator_print_region_info, res_zones[HIGHMEM_ZONE],
	                                res_count[HIGHMEM_ZONE]);
	vga_printf("------------------------------------------\n");
}

void boot_allocator_init(multiboot_tag_mmap_t *mmap, uint8_t *mmap_end)
{
	BOOT_ALLOC_FREE_COUNT(&bootmem)     = 0;
	BOOT_ALLOC_RESERVED_COUNT(&bootmem) = 0;
	BOOT_ALLOC_HOLE_COUNT(&bootmem)     = 0;
	bootmem.num_allocations             = 0;
	// Zone VGA/BIOS_ROM (0xa0000-0x100000)
	boot_allocator_add_region(&bootmem, 0xa0000, 0x100000, RESERVED_MEMORY);
	// Zone Low memory
	const gdtr_t *gdtr = get_gdtr();
	const idtr_t *idtr = get_idtr();
	boot_allocator_add_region(&bootmem, VIRT_TO_PHYS_LINEAR(gdtr->base),
	                          VIRT_TO_PHYS_LINEAR(gdtr->base + gdtr->limit + 1), RESERVED_MEMORY);
	boot_allocator_add_region(&bootmem, VIRT_TO_PHYS_LINEAR(idtr->base),
	                          VIRT_TO_PHYS_LINEAR(idtr->base + idtr->limit + 1), RESERVED_MEMORY);
	boot_allocator_add_region(&bootmem, 0x0, 0x1000, RESERVED_MEMORY);
	// Zone Inconnu --> todo : understand wtf is this shit, marked as problematic at the moment
	boot_allocator_add_region(&bootmem, 0x1000, 0x9fc00, RESERVED_MEMORY);
	// Zone Kernel Code
	boot_allocator_add_region(&bootmem, VIRT_TO_PHYS_LINEAR(kernel_start),
	                          VIRT_TO_PHYS_LINEAR(kernel_end), RESERVED_MEMORY);

	// Zone Boot_info
	boot_allocator_add_region(&bootmem, VIRT_TO_PHYS_LINEAR(mb2info),
	                          VIRT_TO_PHYS_LINEAR(mb2info + mb2info->total_size), RESERVED_MEMORY);

	mb2_mmap_iter(mmap, mmap_end, boot_allocator_reserved_wrapper, false);
	BOOT_ALLOCATOR_SORT_AND_MERGE(bootmem.regions[RESERVED_MEMORY], bootmem.count[RESERVED_MEMORY]);

	mb2_mmap_iter(mmap, mmap_end, boot_allocator_free_handler, true);
	BOOT_ALLOCATOR_SORT_AND_MERGE(bootmem.regions[FREE_MEMORY], bootmem.count[FREE_MEMORY]);

	boot_allocator_fill_gaps_as_holes();
	mb2_mmap_iter(mmap, mmap_end, boot_allocator_init_total_size, false);
	mb2_mmap_iter(mmap, mmap_end, boot_allocator_init_total_size, true);

	total_pages = boot_allocator_get_total_visibale_ram(&bootmem) / PAGE_SIZE;
	boot_allocator_init_zones(free_count, free_zones, FREE_MEMORY);
	boot_allocator_init_zones(res_count, res_zones, RESERVED_MEMORY);
	for (int zone = 0; zone < MAX_ZONE; zone++) {
		BOOT_ALLOCATOR_SORT_AND_MERGE(free_zones[zone], free_count[zone]);
		BOOT_ALLOCATOR_SORT_AND_MERGE(res_zones[zone], res_count[zone]);
	}
}

// Be careful Only DMA/LOWMEM is USABLE otherwise you need to do a temporary mapping
void *boot_alloc(uint32_t size, zone_type zone, bool freeable)
{
	if (bootmem.state == FROZEN) {
		vga_printf("Error: boot allocator is frozen\n");
		return NULL;
	}

	for (int i = free_count[zone] - 1; i >= 0; i--) {
		region_t *reg         = &free_zones[zone][i];
		uint32_t  region_size = reg->end - reg->start;

		if (region_size >= size) {
			if (reg->end - size < reg->start) {
				vga_printf("ERROR: %s: Allocation would exceed region!\n", __func__);
				continue;
			}

			void *ret = (void *)(reg->end - size);
			boot_allocator_add_to_zones(res_count, res_zones, size, reg->end - size);
			reg->end = (uintptr_t)ret;

			if (reg->start == reg->end) {
				*reg = free_zones[zone][--free_count[zone]];
			}
			uint32_t index                    = bootmem.num_allocations;
			bootmem.allocations[index].p_addr = (uintptr_t)ret;
			bootmem.allocations[index].free   = freeable;
			bootmem.allocations[index].size   = size;
			bootmem.num_allocations++;
			return ret;
		}
	}
	vga_printf("Error: %s: No space left on device\n", __func__);
	return NULL;
}

void *boot_alloc_at(uint32_t size, zone_type zone, bool freeable, uintptr_t start, uintptr_t end,
                    int align)
{
	if (bootmem.state == FROZEN || size == 0 || start + size > end) {
		vga_printf("Error: %s: Invalid parameters or allocator frozen.\n", __func__);
		return NULL;
	}

	for (uint32_t i = 0; i < free_count[zone]; i++) {
		region_t *reg = &free_zones[zone][i];

		if (reg->start >= end || reg->end <= start)
			continue;

		uintptr_t alloc_start     = reg->start >= start ? reg->start : start;
		uintptr_t alloc_end_limit = (reg->end < end) ? reg->end : end;
		if (align != -1) {
			alloc_start = ALIGN(alloc_start, align);
		}

		if (alloc_start + size <= alloc_end_limit) {

			uintptr_t alloc_end        = alloc_start + size;
			uintptr_t original_reg_end = reg->end;

			boot_allocator_add_to_zones(res_count, res_zones, size, alloc_start);

			uint32_t index                    = bootmem.num_allocations++;
			bootmem.allocations[index].p_addr = alloc_start;
			bootmem.allocations[index].free   = freeable;
			bootmem.allocations[index].size   = size;

			if (alloc_end < original_reg_end) {
				if (free_count[zone] >= MAX_REGIONS)
					kpanic("No more space for free regions!");
				free_zones[zone][free_count[zone]++] = (region_t){alloc_end, original_reg_end};
			}

			reg->end = alloc_start;
			if (reg->start >= reg->end) {
				*reg = free_zones[zone][--free_count[zone]];
			}
			boot_allocator_sort_regions(free_zones[zone], free_count[zone]);

			return (void *)alloc_start;
		}
	}
	vga_printf("Error: %s: No suitable alloc with %u size, with range %p -> %p \n", __func__, size,
	           start, end);
	return NULL;
}
