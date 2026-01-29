#include <arch/trap_frame.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/boot_allocator.h>
#include <memory/memory.h>
#include <memory/vmm.h>

/*
 * Virtual-to-physical address translation (32-bit paging):
 * - Bits 31-22: Page Directory index
 * - Bits 21-12: Page Table index
 * - Bits 11-0 : Offset within 4KB page
 *
 * See: https://wiki.osdev.org/Paging#32-bit_Paging_(Protected_Mode)
 */

// Defines

#define PD_SLOT                1022
#define FIRST_PAGE_TABLE_VADDR (PD_SLOT << 22)
#define PAGE_DIR_VADDR         ((PD_SLOT << 22) | (PD_SLOT << 12))
#define GET_PT_WITH_INDEX(idx) (FIRST_PAGE_TABLE_VADDR + (idx * PAGE_SIZE))

uintptr_t kpage_dir = 0;

// Internal APIs

// External APIs

void page_fault_handler(trap_frame_t *frame)
{
	uint32_t faulting_address;

	asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

	vga_printf("Faulting address: 0x%x\n", faulting_address);
	vga_printf("Error code: 0x%x\n", frame->err_code);

	if (frame->err_code & 0x1) {
		vga_printf("Cause: Protection violation\n");
	} else {
		vga_printf("Cause: Page not present\n");
	}
	kpanic("Page fault");
}

void vmm_finalize(void)
{
	uintptr_t page_dir_phys =
	    (uintptr_t)boot_alloc_at(PAGE_SIZE, DMA_ZONE, TO_KEEP, 0x0, MiB_SIZE * 4, PAGE_SIZE);
	if (!page_dir_phys)
		kpanic("Failed PD alloc");

	uint32_t *page_dir_ptr = (uint32_t *)page_dir_phys;
	ft_bzero(page_dir_ptr, PAGE_SIZE);

	const uint32_t pt_count = 224;
	uintptr_t pt_pool_phys  = (uintptr_t)boot_alloc_at(pt_count * PAGE_SIZE, DMA_ZONE, TO_KEEP, 0x0,
	                                                   MiB_SIZE * 4, PAGE_SIZE);
	if (!pt_pool_phys)
		kpanic("Failed PT pool alloc");

	for (uint32_t i = 0; i < pt_count; i++) {
		uint32_t pde_idx = 768 + i;

		uintptr_t current_pt_phys_addr = pt_pool_phys + (i * PAGE_SIZE);

		page_dir_ptr[pde_idx] = current_pt_phys_addr | PDE_PRESENT_BIT | PDE_RW_BIT;

		uint32_t *current_pt_ptr = (uint32_t *)current_pt_phys_addr;
		ft_bzero(current_pt_ptr, PAGE_SIZE);

		for (uint32_t j = 0; j < 1024; j++) {
			uint32_t p_addr   = (i * 4 * MiB_SIZE) + (j * PAGE_SIZE);
			current_pt_ptr[j] = p_addr | PTE_PRESENT_BIT | PTE_RW_BIT;
		}
	}
	kpage_dir = page_dir_phys;
	paging_reload_cr3(page_dir_phys);
}

bool vmm_map_page(uintptr_t page_dir_phys, uintptr_t v_addr, uintptr_t p_addr, uint32_t flags)
{
	uint32_t pde_idx = GET_PDE_INDEX(v_addr);
	uint32_t pte_idx = GET_PTE_INDEX(v_addr);

	uint32_t *pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(page_dir_phys);
	uint32_t  pde     = pd_virt[pde_idx];

	uint32_t *pt_virt;
	bool      needs_cr3_reload = false;

	if (!(pde & PDE_PRESENT_BIT)) {
		uintptr_t pt_phys = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
		if (!pt_phys)
			return false;

		pt_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(pt_phys);
		ft_bzero(pt_virt, PAGE_SIZE);

		pd_virt[pde_idx] = pt_phys | PDE_PRESENT_BIT | PDE_RW_BIT | PDE_US_BIT;
		needs_cr3_reload = true;
	} else {
		uintptr_t pt_phys = GET_ENTRY_ADDR(pde);
		pt_virt           = (uint32_t *)PHYS_TO_VIRT_LINEAR(pt_phys);
	}
	pt_virt[pte_idx] = p_addr | flags;

	uintptr_t current_pd = get_current_page_directory_phys();
	if (current_pd = page_dir_phys) {
		if (needs_cr3_reload) {
			paging_reload_cr3(page_dir_phys);
		} else {
			paging_invalid_TLB_addr(v_addr);
		}
	}
	return true;
}

bool vmm_unmap_page(uintptr_t page_dir_phys, uintptr_t v_addr)
{
	uint32_t pde_idx = GET_PDE_INDEX(v_addr);
	uint32_t pte_idx = GET_PTE_INDEX(v_addr);

	uint32_t *pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(page_dir_phys);
	uint32_t  pde     = pd_virt[pde_idx];

	if (!(pde & PDE_PRESENT_BIT))
		return false;

	uintptr_t pt_phys = GET_ENTRY_ADDR(pde);
	uint32_t *pt_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(pt_phys);

	if (!(pt_virt[pte_idx] & PTE_PRESENT_BIT))
		return false;

	pt_virt[pte_idx] = 0;
	paging_invalid_TLB_addr(v_addr);

	bool is_pt_empty = true;
	for (int i = 0; i < 1024; i++) {
		if (pt_virt[i] & PTE_PRESENT_BIT) {
			is_pt_empty = false;
			break;
		}
	}

	if (is_pt_empty) {
		pd_virt[pde_idx] = 0;
		buddy_free_block((void *)pt_phys);
		if (page_dir_phys == get_current_page_directory_phys())
			paging_reload_cr3(page_dir_phys);
	}

	return true;
}

uintptr_t vmm_get_mapping(uintptr_t page_dir_phys, uintptr_t v_addr)
{
	uint32_t pde_idx = GET_PDE_INDEX(v_addr);
	uint32_t pte_idx = GET_PTE_INDEX(v_addr);

	uint32_t *pd_ptr = (uint32_t *)PHYS_TO_VIRT_LINEAR(page_dir_phys);
	uint32_t  pde    = pd_ptr[pde_idx];

	if (!(pde & PDE_PRESENT_BIT)) {
		return 0;
	}

	uintptr_t page_table = GET_ENTRY_ADDR(pde);
	uint32_t *pt_ptr     = (uint32_t *)PHYS_TO_VIRT_LINEAR(page_table);
	uint32_t  pte        = pt_ptr[pte_idx];

	if (!(pte & PTE_PRESENT_BIT)) {
		return 0;
	}

	uintptr_t page_phys_base = GET_ENTRY_ADDR(pte);
	uintptr_t offset         = v_addr & 0xFFF;

	return page_phys_base + offset;
}

uintptr_t vmm_get_kernel_directory(void) { return kpage_dir; }
