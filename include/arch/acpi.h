#pragma once

#include "../arch/register.h"
#include "../utils/compiler.h"
#include "io.h"

static __always_inline __noreturn void halt(void)
{
	clean_registers();
	__asm__ volatile("cli\n"
	                 "hlt");
	unreachable();
}

static __always_inline __noreturn void shutdown(void)
{
	// Works in newer versions of QEMU
	outw(0x604, 0x2000);
	halt();
}

static __always_inline __noreturn void reboot(void)
{
	while (inb(0x64) & 0x02)
		;
	outb(0x64, 0xFE);
	halt();
}
