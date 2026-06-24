#pragma once

#include <types.h>

void   vmalloc_init(void);
void  *vmalloc(size_t size);
size_t vsize(void *ptr);
void   vfree(void *ptr);
