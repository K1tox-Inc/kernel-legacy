#include <arch/trap_frame.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/boot_allocator.h>
#include <memory/memory.h>
#include <memory/vma.h>
#include <memory/vmm.h>
#include <proc/task.h>
#include <utils/kmacro.h>

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

enum PageFaultCauses {
	PFC_PRESENT       = 1 << 0,
	PFC_WRITE         = 1 << 1,
	PFC_USER          = 1 << 2,
	PFC_RESERVED      = 1 << 3,
	PFC_INSTR_FETCH   = 1 << 4,
	PFC_PROT_KEY      = 1 << 5,
	PFC_SSTACK        = 1 << 6,
	PFC_SOFT_GUARD_EX = 1 << 15,
};

void page_fault_handler(struct trap_frame *frame)
{
#define caused_by(cause) FLAG_IS_SET(frame->err_code, cause)

	void *faulting_address;
	__asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));

	struct task    *cur_task   = task_get_current_task();
	struct vm_area *fault_area = vma_find_by_addr(faulting_address, &cur_task->vma_areas);

	if (fault_area && fault_area->state == VM_AREA_LAZY) {
		if (!vma_map_area(fault_area, cur_task->cr3))
			kpanic("Failed to map lazy VM area");
		fault_area->state = VM_AREA_ALLOCATED;
		return;
	}

#ifndef NDEBUG

	vga_printf("\n   === PAGE FAULT ===\n");
	vga_printf("Faulting address: %p\n", faulting_address);
	vga_printf("Instr. Pointer:   %p\n", frame->eip);
	vga_printf("Page Directory:   %p\n", cur_task->cr3);
	vga_printf("Page Dir. Index:  %d (+%p)\n", GET_PDE_INDEX(faulting_address),
	           GET_PDE_INDEX(faulting_address) * sizeof(uint32_t));
	vga_printf("Page Table:       %p\n", GET_ENTRY_ADDR(((uint32_t *)PHYS_TO_VIRT_LINEAR(
	                                         cur_task->cr3))[GET_PDE_INDEX(faulting_address)]));
	vga_printf("Page Table Index: %d (+%p)\n", GET_PTE_INDEX(faulting_address),
	           GET_PTE_INDEX(faulting_address) * sizeof(uint32_t));
	vga_printf("Error code:       0x%x\n", frame->err_code);
	vga_printf("Task:             %t\n", cur_task);
	vga_printf("Causes:\n");

# define pr_err_cause(cause, msg)                                                                  \
	 if (caused_by(cause))                                                                         \
		 vga_printf("%s\n", msg);

	if (caused_by(PFC_PRESENT)) {
		vga_printf(" - Present (P): The page fault was caused by a "
		           "page-protection violation.\n");
	} else {
		vga_printf(" - Present (P): The page fault was caused by a non-present page.\n");
	}

	pr_err_cause(PFC_WRITE, " - Write (W): The page fault was caused by a write "
	                        "access.")

	    pr_err_cause(PFC_USER,
	                 " - User (U): The page fault was caused while CPL = 3. This "
	                 "does not necessarily mean that the page fault was a privilege violation.");

	pr_err_cause(PFC_RESERVED,
	             " - Reserved write (R): One or more page directory "
	             "entries contain reserved bits which are set to 1. This only applies "
	             "when the PSE or PAE flags in CR4 are set to 1.");

	pr_err_cause(PFC_INSTR_FETCH,
	             " - Instruction Fetch (I): The page fault was caused by an instruction "
	             "fetch. This only applies when the No-Execute bit is supported and enabled.");

	pr_err_cause(PFC_PROT_KEY,
	             " - Protection key (PK): The page fault was caused by a "
	             "protection-key violation. The PKRU register (for user-mode accesses) or PKRS "
	             "MSR (for supervisor-mode accesses) specifies the protection key rights.");

	pr_err_cause(PFC_SSTACK, " - Shadow stack (SS): The page fault was caused by a "
	                         "shadow stack access.");

	pr_err_cause(PFC_SOFT_GUARD_EX,
	             " - Software Guard Extensions (SGX): The fault was "
	             "due to an SGX violation. The fault is unrelated to ordinary paging.");

# undef pr_err_cause

	vga_printf("\n");

#endif

#undef caused_by

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
	uintptr_t pt_phys;
	bool      needs_cr3_reload = false;

	if (!(pde & PDE_PRESENT_BIT)) {
		pt_phys = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
		if (!pt_phys)
			return false;

		pt_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(pt_phys);
		ft_bzero(pt_virt, PAGE_SIZE);

		pd_virt[pde_idx] = pt_phys | PDE_PRESENT_BIT | PDE_RW_BIT | (flags & PDE_US_BIT);

		needs_cr3_reload = true;
	}

	else {
		if ((flags & PDE_US_BIT) && !(pde & PDE_US_BIT)) {
			pd_virt[pde_idx] |= PDE_US_BIT;
			needs_cr3_reload = true;
		}

		pt_phys = GET_ENTRY_ADDR(pde);
		pt_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(pt_phys);
	}

	pt_virt[pte_idx] = p_addr | flags;

	uintptr_t current_pd = get_current_page_directory_phys();
	if (current_pd == page_dir_phys) {
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

#define VMM_KMAP_VADDR 0xFFFFE000

void *vmm_kmap(uintptr_t p_addr)
{
	if (!vmm_map_page(get_current_page_directory_phys(), VMM_KMAP_VADDR, p_addr,
	                  PTE_PRESENT_BIT | PTE_RW_BIT))
		return NULL;
	return (void *)VMM_KMAP_VADDR;
}

void vmm_kunmap(void) { vmm_unmap_page(get_current_page_directory_phys(), VMM_KMAP_VADDR); }

void vmm_free_pt_range(uintptr_t pd_phys, uint32_t pde_start, uint32_t pde_end)
{
	uint32_t *pd = (uint32_t *)PHYS_TO_VIRT_LINEAR(pd_phys);

	for (uint32_t i = pde_start; i < pde_end; i++) {
		if (pd[i] & PDE_PRESENT_BIT) {
			buddy_free_block((void *)GET_ENTRY_ADDR(pd[i]));
			pd[i] = 0;
		}
	}
}

void vmm_destroy_user_pd(uintptr_t pd_phys)
{
	vmm_free_pt_range(pd_phys, 0, 768);
	buddy_free_block((void *)pd_phys);
}

int vmm_verify_range_flags(uint32_t *pd_virt, const void *vaddr, unsigned long n,
                           uint32_t pde_flags, uint32_t pte_flags)
{
	if (n == 0)
		return 0;

	uint32_t pde_start       = GET_PDE_INDEX((uintptr_t)vaddr);
	uint32_t pde_end         = GET_PDE_INDEX((uintptr_t)vaddr + n - 1);
	size_t   pde_range       = pde_end - pde_start + 1;
	size_t   pages_to_verify = DIV_ROUND_UP(((uintptr_t)vaddr % PAGE_SIZE) + n, PAGE_SIZE);
	size_t   remaining_page  = pages_to_verify;

	pde_flags |= PDE_PRESENT_BIT;
	pte_flags |= PTE_PRESENT_BIT;

	for (size_t i = 0; i < pde_range; i++) {

		uint32_t pde = pd_virt[pde_start + i];
		if ((pde & pde_flags) != pde_flags)
			return -1;

		uintptr_t cur_vaddr = (uintptr_t)vaddr + (i * (1024 * PAGE_SIZE));
		uint32_t *pt_virt   = PHYS_TO_VIRT_LINEAR(GET_ENTRY_ADDR(pde));

		size_t j = (i == 0) ? GET_PTE_INDEX(cur_vaddr) : 0;

		while (remaining_page > 0 && j < 1024) {
			if ((pt_virt[j] & pte_flags) != pte_flags)
				return -1;
			j++;
			remaining_page--;
		}
	}
	return 0;
}
