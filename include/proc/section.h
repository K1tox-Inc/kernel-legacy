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

// #define USER_HEAP_DEFAULT_START     0x0804A000       twice is not use bcof get_next_section_start
// #define USER_DATA_DEFAULT_START     0x08049000                       macro can handle it
// dynamicaly

#define USER_TEXT_START 0x08048000 // legacy for ABI system
#define USERSPACE_START 0x00000000

#define USER_SECTION_RO (PTE_PRESENT_BIT | PTE_US_BIT)
#define USER_SECTION_RW (PTE_PRESENT_BIT | PTE_US_BIT | PTE_RW_BIT)

// ============================================================================
// STRUCTS & MACROS
// ============================================================================

typedef struct section {
	uintptr_t v_addr;
	uintptr_t p_addr;
	uintptr_t data_start;
	uint32_t  data_size;
	uint32_t  mapping_size;
	uint32_t  flags;
} section_t;

// ============================================================================
// EXTERNAL APIs
// ============================================================================

struct io_stream;

bool section_init_from_buffer(section_t *sec, uintptr_t v_addr, const void *buffer, uint32_t size,
                              uint32_t flags);

struct io_stream *section_create_reader(section_t *sec);
