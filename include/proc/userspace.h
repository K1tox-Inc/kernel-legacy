#pragma once

#include <memory/memory.h>
#include <types.h>
#include <utils/kmacro.h>

// ============================================================================
// MEMORY LAYOUT DEFINITIONS
// ============================================================================

#define KERNEL_END   0xFFFFFFFF
#define KERNEL_START (KERNEL_VADDR_BASE)

#define USER_STACK_START (KERNEL_START - PAGE_SIZE)
#define USER_STACK_SIZE  8192
#define USER_STACK_END   (USER_STACK_START - USER_STACK_SIZE)

#define USER_HEAP_MAX  (USER_STACK_END - PAGE_SIZE)
#define USER_HEAP_SIZE 0

// #define USER_HEAP_DEFAULT_START     0x0804A000       twice is not use bcof get_next_section_start
// #define USER_DATA_DEFAULT_START     0x08049000                       macro can handle it
// dynamicaly

#define USER_TEXT_START 0x08048000 // legacy for ABI system
#define USERSPACE_START 0x00000000

// ============================================================================
// STRUCTS & MACROS
// ============================================================================

typedef struct section {
    uintptr_t v_addr;
    uintptr_t src_ptr;
    uint32_t  size;
    uint32_t  flags;
} section_t;

#define get_next_section_start(start, size) ALIGN(((start) + (size)), PAGE_SIZE)

// ============================================================================
// EXTERNAL APIs
// ============================================================================

uintptr_t userspace_create_new(section_t *text, section_t *data, section_t *stack);
