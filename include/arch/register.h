#pragma once

#include <types.h>

typedef struct {
	uint32_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} __attribute__((packed)) REGISTERS;

static inline void clean_registers(void)
{
	__asm__ volatile("xor %%eax, %%eax\n\t"
	                 "xor %%ebx, %%ebx\n\t"
	                 "xor %%ecx, %%ecx\n\t"
	                 "xor %%edx, %%edx\n\t"
	                 "xor %%esi, %%esi\n\t"
	                 "xor %%edi, %%edi\n\t"
	                 :
	                 :
	                 : "eax", "ebx", "ecx", "edx", "esi", "edi");
}
