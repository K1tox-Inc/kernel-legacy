#include <drivers/vga.h>
#include <syscall/syscall.h>

SYSCALL_DEFINE1(write, char, c)
{
	vga_printf("From userspace : %c\n", c);
	return 0;
}
