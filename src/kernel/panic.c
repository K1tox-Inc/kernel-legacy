#include <arch/acpi.h>
#include <drivers/tty.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <types.h>

static uint8_t stack_snapshot[4096];

void __assert_fail(const char *expr, const char *file, size_t line)
{
	vga_disable_cursor();
	tty_framebuffer_set_screen_mode(current_tty, VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_WHITE));
	vga_printf("Assertion failed:%s:%u: %s\n", file, line, expr);
	halt();
}

void save_stack(void)
{
	uint32_t *ebp, *esp;
	__asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
	__asm__ volatile("mov %%esp, %0" : "=r"(esp));

	for (int i = 0; i < 4096 && esp < ebp; i++)
		stack_snapshot[i] = *(uint8_t *)(esp++);
}

void memory_dump(uint32_t addr_start, uint32_t addr_end)
{
	uint32_t addr = addr_end;
	while (addr >= addr_start) {
		if (addr % 8 == 0 || addr == addr_start)
			vga_printf("%p:\t", addr);
		if (*(uint8_t *)addr < 0x10)
			vga_printf("0");
		vga_printf("%x ", *(uint8_t *)(addr--));
		if (addr % 8 == 0)
			vga_printf("\n");
	}
	if (addr % 8)
		vga_printf("\n");
}

void print_stack_frame(void)
{
	uint32_t *ebp, *esp;

	__asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
	__asm__ volatile("mov %%esp, %0" : "=r"(esp));

	vga_printf("Stack trace:\n");
	memory_dump((uint32_t)esp, (uint32_t)ebp);

	vga_printf("ESP = %p | EBP = %p\n", esp, ebp);
	uint32_t eip = *(ebp + 1);
	vga_printf("Return address: 0x%x\n", eip);
}
