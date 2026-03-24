#pragma once

#include <arch/acpi.h>
#include <drivers/tty.h>
#include <proc/lock.h>
#include <types.h>

#define kpanic(msg, ...)                                                                           \
	do {                                                                                           \
		lock_irq();                                                                                \
		tty_framebuffer_set_screen_mode(current_tty, VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_WHITE));   \
		vga_disable_cursor();                                                                      \
		vga_printf("\n------------------------------------\n");                                    \
		print_stack_frame();                                                                       \
		vga_printf("------------------------------------\n");                                      \
		vga_printf("PANIC in %s:%d\n", __FILE__, __LINE__);                                        \
		vga_printf(msg, ##__VA_ARGS__);                                                            \
		halt();                                                                                    \
	} while (0)

void print_stack_frame(void);
