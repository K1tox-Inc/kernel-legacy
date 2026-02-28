#include <arch/x86.h>
#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <kernel/mb2_info.h>
#include <memory/buddy.h>
#include <memory/memory.h>
#include <memory/page.h>
#include <memory/slab.h>
#include <memory/vmalloc.h>
#include <memory/vmm.h>

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
__attribute__((section(".stack"))) uint8_t kernel_stack[16 * 1024] __attribute__((aligned(16)));

void call_constructors(void)
{
	size_t count = (size_t)(__init_array_end - __init_array_start);
	for (size_t i = 0; i < count; i++) {
		__init_array_start[i]();
	}
}

void x86_init(void)
{
	gdt_init();
	idt_init();
	idt_register_interrupt_handlers(14, (irqHandler)page_fault_handler);
	mb2_init();
	vmm_finalize();
	page_descriptor_init();
	buddy_init();
	slab_init();
	vmalloc_init();
	call_constructors();
	keyboard_init();
	ttys_init();
}
