#pragma once

// INCLUDES

// DEFINE AND MACRO

// Defines

//  Page Table Entry (PTE)
enum Page_Table_Entry {
	// Bit 0  : Present         (page is in memory)
	PTE_PRESENT_BIT = 0b0000000000000001,

	// Bit 1  : Read/Write      (1 = writable, 0 = read-only)
	PTE_RW_BIT = 0b0000000000000010,

	// Bit 2  : User/Supervisor (1 = user mode, 0 = supervisor/kernel)
	PTE_US_BIT = 0b0000000000000100,

	// Bit 3  : Write-Through   (1 = write-through caching, 0 = write-back)
	PTE_PWT_BIT = 0b0000000000001000,

	// Bit 4  : Cache-Disable   (1 = no cache for this page)
	PTE_PCD_BIT = 0b0000000000010000,

	// Bit 5  : Accessed        (set by CPU on access)
	PTE_ACCESSED_BIT = 0b0000000000100000,

	// Bit 6  : Dirty           (set by CPU on write)
	PTE_DIRTY_BIT = 0b0000000001000000,

	// Bit 7  : PAT             (Page Attribute Table extension)
	PTE_PAT_BIT = 0b0000000010000000,

	/*
	 * Bit 8  : Global          (1 = global page, not flushed
	 * from TLB on context switch)
	 */
	PTE_GLOBAL_BIT = 0b0000000100000000,

	// Bit 9  : Available       (for OS use)
	PTE_AVAIL1_BIT = 0b0000001000000000,

	// Bit 10 : Available       (for OS use)
	PTE_AVAIL2_BIT = 0b0000010000000000,

	/*
	 * Bit 11 : Available       (for OS use)
	 * Bits 12-31 : Physical address of the page (aligned on 4KiB)
	 */
	PTE_AVAIL3_BIT = 0b0000100000000000
};

// Page Directory Entry (PDE)
enum Page_Directory_Entry {
	// Bit 0  : Present
	PDE_PRESENT_BIT = 0b0000000000000001,

	// Bit 1  : Read/Write
	PDE_RW_BIT = 0b0000000000000010,

	// Bit 2  : User/Supervisor
	PDE_US_BIT = 0b0000000000000100,

	// Bit 3  : Write-Through (PWT)
	PDE_PWT_BIT = 0b0000000000001000,

	// Bit 4  : Cache-Disable (PCD)
	PDE_PCD_BIT = 0b0000000000010000,

	// Bit 5  : Accessed
	PDE_ACCESSED_BIT = 0b0000000000100000,

	// Bit 6  : Always 0 (must be zero for 4KiB pages)
	PDE_ALWAYS0_BIT = 0b0000000001000000,

	// Bit 7  : Page Size (0 = 4KiB, 1 = 4MiB)
	PDE_PS_BIT = 0b0000000010000000,

	// Bit 8  : Available for OS
	PDE_AVAIL1_BIT = 0b0000000100000000,

	// Bit 9  : Available for OS
	PDE_AVAIL2_BIT = 0b0000001000000000,

	// Bit 10 : Available for OS
	PDE_AVAIL3_BIT = 0b0000010000000000,

	/*
	 * Bit 11 : Available for OS
	 * Bits 12-31 : Physical address of the page table (aligned on 4KiB)
	 */
	PDE_AVAIL4_BIT = 0b0000100000000000
};

// Macros

#define GET_PDE_INDEX(vaddr)  (vaddr >> 22)
#define GET_PTE_INDEX(vaddr)  ((vaddr >> 12) & 0x3FF)
#define GET_ENTRY_ADDR(entry) ((entry & ~0xFFF))

// STRUCT

// Enums

// Structures

// Typedefs

// VARIABLES GLOBALES

extern uintptr_t current_page_dir;

// EXTERNAL APIs

void      page_fault_handler(REGISTERS reg, int interrupt, int error);
uintptr_t vmm_get_mapping(uintptr_t page_dir_phys, uintptr_t v_addr);
bool      vmm_map_page(uintptr_t page_dir_phys, uintptr_t v_addr, uintptr_t p_addr, uint32_t flags);
bool      vmm_unmap_page(uintptr_t page_dir_phys, uintptr_t v_addr);

static inline uintptr_t get_current_page_directory_phys(void)
{
	uintptr_t pd_phys;
	asm volatile("mov %%cr3, %0" : "=r"(pd_phys));
	return pd_phys;
}

static inline void paging_reload_cr3(uintptr_t pd_phys_addr)
{
	asm volatile("mov %0, %%cr3" : : "r"(pd_phys_addr) : "memory");
}

static inline void paging_invalid_TLB_addr(uint32_t addr)
{
	__asm__ volatile("invlpg (%0)" ::"r"(addr));
}

static inline void paging_flush_TLB(void)
{
	uint32_t cr3;
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	__asm__ volatile("mov %0, %%cr3" ::"r"(cr3));
}

static inline void paging_disable_ro(void)
{
	uint32_t cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	asm volatile("mov %0, %%cr0" ::"r"(cr0 & ~0x10000));
}

static inline void paging_enable_ro(void)
{
	uint32_t cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	asm volatile("mov %0, %%cr0" ::"r"(cr0 | 0x10000));
}
