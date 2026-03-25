#pragma once

#include <memory/memory.h>
#include <types.h>

static inline bool access_ok(const void *ptr, size_t size)
{
	uintptr_t start = (uintptr_t)ptr;
	uintptr_t end   = start + size;

	if (end < start)
		return false;
	if (end > KERNEL_VADDR_BASE)
		return false;

	return true;
}
