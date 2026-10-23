#include <syscalls/syscalls.h>
#include <types.h>

SYSCALL_DEFINE3(getdents64, int, fd, void *, buf, size_t, count)
{
	(void)fd;
	(void)buf;
	(void)count;

	return 0;
}
