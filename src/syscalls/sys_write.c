#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/usercopy.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE3(write, int, fd, const char *, str, size_t, size)
{
	(void)fd;

	if (size > MAX_KMALLOC_SIZE)
		return -EINVAL;
	char *dup = kmalloc(size + 1, __GFP_KERNEL);
	if (!dup)
		return -ENOMEM;
	int ret = copy_from_user(dup, str, size);
	if (ret) {
		kfree(dup);
		return -EFAULT;
	}
	dup[size] = 0;

	vga_printf("%s", dup);

	kfree(dup);

	return size;
}
