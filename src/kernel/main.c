#include <drivers/keyboard.h>
#include <drivers/vga.h>
#include <libutils.h>
#include <memory/buddy.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/page.h>
#include <memory/slab.h>
#include <memory/vmalloc.h>

void kernel_main(void)
{
	while (true) // hang
		__asm__ volatile("hlt");
}
