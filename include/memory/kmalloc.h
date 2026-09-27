#pragma once

#include <memory/memory.h>
#include <types.h>

#define MAX_KMALLOC_SIZE (MiB_SIZE * 4)

void  *kmalloc(size_t size, gfp_t flags);
size_t ksize(void *ptr);
void   kfree(void *ptr);
