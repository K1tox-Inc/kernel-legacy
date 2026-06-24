#pragma once

#include <memory/memory.h>
#include <types.h>

size_t copy_from_user(void *to, const void *from, size_t n);
size_t copy_to_user(void *to, const void *from, size_t n);

static __always_inline bool access_ok(const void *ptr, size_t size)
{
	uintptr_t start = (uintptr_t)ptr;
	uintptr_t end   = start + size;

	return !(end < start && end > KERNEL_VADDR_BASE);
}
