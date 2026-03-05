#include <arch/tss.h>
#include <arch/x86.h>
#include <libk.h>
#include <memory/memory.h>
#include <types.h>

// Structs & Defines

#define GDT_MAX_ENTRIES 6

extern uint8_t kernel_stack_top[];
struct tss     g_tss = {0};

enum Gdt_Flags {
	RESERVED_BIT     = 0b0001, // Bit 0: Réservé, doit être à 0
	LONG_MODE_BIT    = 0b0010, // Bit 1: 1=segment 64-bit (mode long), 0=mode protégé standard
	SEGMENT_SIZE_BIT = 0b0100, // Bit 2: 0=16-bit segment, 1=32-bit segment
	GRANULARITY_BIT  = 0b1000  // Bit 3: 0=limite en octets, 1=limite en pages de 4Ko
};

struct segment_descriptor {
	uint16_t limit_low;      // 0  -> 15
	uint16_t base_low;       // 16 -> 31
	uint8_t  base_middle;    // 32 -> 39
	uint8_t  access;         // 40 -> 47
	uint8_t  limit_high : 4; // 48 -> 51
	uint8_t  flags : 4;      // 51 -> 55
	uint8_t  base_high;      // 55 -> 63
} __attribute__((packed));

// Code

gdtr_t                    gdtr;
struct segment_descriptor gdt_entries[GDT_MAX_ENTRIES];

static inline void gdt_set_entry(struct segment_descriptor *gdt_entry, uint32_t base,
                                 uint32_t limit, uint8_t access, uint8_t flags)
{
	gdt_entry->limit_low   = (limit & 0xFFFF);
	gdt_entry->base_low    = (base & 0xFFFF);
	gdt_entry->base_middle = (base >> 16) & 0xFF;
	gdt_entry->access      = access;
	gdt_entry->limit_high  = (limit >> 16) & 0x0F;
	gdt_entry->flags       = flags & 0x0F;
	gdt_entry->base_high   = (base >> 24) & 0xFF;
}

const gdtr_t *get_gdtr(void) { return &gdtr; }

void set_tss_to_kstack_top(uintptr_t stack_top) { g_tss.esp0 = stack_top; }

void gdt_init(void)
{
#define GDT_ENTRY(indx)   (gdt_entries + (indx))
#define GDT_COMMON_ACCESS (ACCESS_BIT | RW_BIT | PRESENT_BIT | DESCRIPTOR_TYPE)
#define GDT_FLAGS         (SEGMENT_SIZE_BIT | GRANULARITY_BIT)

	gdt_set_entry(GDT_ENTRY(GDT_IDX_NULL), 0x00, 0x00, 0x00, 0x00);

#define GDT_KERNEL_ACCESS (GDT_COMMON_ACCESS | DPL_RING0)

	// -------------- Kernel descriptors --------------
	gdt_set_entry(GDT_ENTRY(GDT_IDX_KERNEL_CODE), 0x00, 0xFFFFFF,
	              GDT_KERNEL_ACCESS | EXECUTABLE_BIT,
	              GDT_FLAGS); // Kernel Code Segment
	gdt_set_entry(GDT_ENTRY(GDT_IDX_KERNEL_DATA), 0x00, 0xFFFFFF, GDT_KERNEL_ACCESS,
	              GDT_FLAGS); // Kernel Data Segment

#undef GDT_KERNEL_ACCESS

#define GDT_USER_ACCESS (GDT_COMMON_ACCESS | DPL_RING3)

	// -------------- User descriptors --------------
	gdt_set_entry(GDT_ENTRY(GDT_IDX_USER_CODE), 0x00, 0xFFFFFF, GDT_USER_ACCESS | EXECUTABLE_BIT,
	              GDT_FLAGS); // User Code Segment
	gdt_set_entry(GDT_ENTRY(GDT_IDX_USER_DATA), 0x00, 0xFFFFFF, GDT_USER_ACCESS,
	              GDT_FLAGS); // User Data Segment

#undef GDT_USER_ACCESS

	gdt_set_entry(GDT_ENTRY(GDT_IDX_TSS), (uint32_t)&g_tss, sizeof(struct tss) - 1,
	              PRESENT_BIT | DPL_RING0 | EXECUTABLE_BIT | ACCESS_BIT, 0x00);

#undef GDT_FLAGS
#undef GDT_ENTRY
#undef GDT_COMMON_ACCESS

	gdtr.limit = (sizeof(struct segment_descriptor) * GDT_MAX_ENTRIES) - 1;
	gdtr.base  = (uint32_t)gdt_entries;

	g_tss.ss0   = 0x10;
	g_tss.esp0  = (uintptr_t)kernel_stack_top;
	g_tss.iomap = sizeof(struct tss);

	// Register the GDT in the CPU
	__asm__ volatile("lgdtl (gdtr)");

	// Reload segment registers
	__asm__ volatile("movw $0x10, %ax \n" // Kernel data segment
	                 "movw %ax, %ds \n"
	                 "movw %ax, %es \n"
	                 "movw %ax, %fs \n"
	                 "movw %ax, %gs \n"
	                 "movw %ax, %ss \n"
	                 "ljmp $0x08, $next \n" // Long jump to reload CS
	                 "next:");

	// Register TSS segment
	__asm__ volatile("ltr %%ax" : : "a"(0x28));
}
