#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <syscalls/syscalls.h>

SYSCALL_DEFINE3(write, int, fd, const char *, str, size_t, size)
{
	(void)fd;

	char *dup = kmalloc(size, __GFP_KERNEL);
	ft_memcpy(dup, str, size);
	dup[size] = 0;

	vga_printf("%s", dup);

	kfree(dup);

	return 0;
}
