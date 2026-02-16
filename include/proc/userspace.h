#pragma once

#include <types.h>
#include <proc/section.h>
#include <utils/kmacro.h>
#include <memory/memory.h>

struct task;

// ============================================================================
// STRUCTS & MACROS
// ============================================================================

#define get_next_section_start(start, size) ALIGN(((start) + (size)), PAGE_SIZE)
#define get_next_section_start_after_page_guard(start, size)                                       \
	ALIGN(((start) + (size + PAGE_SIZE)), PAGE_SIZE)
#define get_prev_section_start(end, size) ALIGN_DOWN(((end) - (size)), PAGE_SIZE)

// ============================================================================
// EXTERNAL APIs
// ============================================================================

bool userspace_create_new(section_t *text, section_t *data, struct task *new_proccess);
