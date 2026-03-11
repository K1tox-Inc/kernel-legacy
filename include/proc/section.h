#pragma once

#include <memory/memory.h>
#include <types.h>
#include <utils/kmacro.h>

// ============================================================================
// MEMORY LAYOUT DEFINITIONS
// ============================================================================

#define KERNEL_END   0xFFFFFFFF
#define KERNEL_START (KERNEL_VADDR_BASE)

#define DEFAULT_STACK_SIZE 8192
#define USER_STACK_START   (KERNEL_START - PAGE_SIZE)
#define USER_STACK_END     (USER_STACK_START - DEFAULT_STACK_SIZE - PAGE_SIZE)

#define USER_HEAP_MAX  (USER_STACK_END - PAGE_SIZE)
#define USER_HEAP_SIZE 0

// Neither is used because the `get_next_section_start` macro can handle them dynamically
// #define USER_HEAP_DEFAULT_START     0x0804A000
// #define USER_DATA_DEFAULT_START     0x08049000

#define USER_TEXT_START 0x08048000 // Legacy for ABI system
#define USERSPACE_START 0x00000000

#define USER_SECTION_RO (PTE_PRESENT_BIT | PTE_US_BIT)
#define USER_SECTION_RW (PTE_PRESENT_BIT | PTE_US_BIT | PTE_RW_BIT)

// ============================================================================
// STRUCTS & MACROS
// ============================================================================

struct section {
	uintptr_t v_addr;
	uintptr_t p_addr;
	uintptr_t data_start;
	uint32_t  data_size;
	uint32_t  mapping_size;
	uint32_t  flags;
};

// ============================================================================
// EXTERNAL APIs
// ============================================================================

struct io_stream;

bool section_init_from_buffer(struct section *sec, uintptr_t v_addr, const void *buffer,
                              uint32_t size, uint32_t flags);

struct io_stream *section_create_reader(struct section *sec);
