#include <syscalls/syscalls.h>
#include <types.h>
#include <uapi/errno.h>

SYSCALL_DEFINE3(getdents64, int, fd, void *, buf, size_t, count)
{
	(void)fd;
	(void)buf;
	(void)count;

	// TODO: Call `vfs_iterate` once there are file descriptors (#122)
	return -ENOSYS;
}
