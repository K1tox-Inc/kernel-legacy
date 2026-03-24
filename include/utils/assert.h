#pragma once

#include <arch/acpi.h>
#include <drivers/tty.h>
#include <utils/kmacro.h>

#define __assert_fail(expr)                                                                        \
	({                                                                                             \
		vga_disable_cursor();                                                                      \
		tty_framebuffer_set_screen_mode(current_tty, VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_WHITE));   \
		log("%s(): Assertion `%s' failed.", __func__, #expr);                                      \
		halt();                                                                                    \
	})

#define static_assert _Static_assert

#ifndef NDEBUG

# define assert(expr) ((expr) ? (void)(0) : __assert_fail(expr))

#else

# define assert(expr)

#endif
