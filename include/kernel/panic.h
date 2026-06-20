#pragma once

#include <types.h>

#define kpanic(fmt, ...)                                                                           \
	__kpanic("PANIC in %s:%d\n"                                                                    \
	         "%s: " fmt "\n",                                                                      \
	         __FILE__, __LINE__, __func__, ##__VA_ARGS__)

void __noreturn __kpanic(const char *fmt, ...);
