#pragma once

#include <memory/memory.h>
#include <types.h>

unsigned long copy_from_user(void *to, const void *from, unsigned long n);
unsigned long copy_to_user(void *to, const void *from, unsigned long n);

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
