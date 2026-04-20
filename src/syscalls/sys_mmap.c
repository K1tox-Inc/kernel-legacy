#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

enum mmap_prot {
	PROT_NONE  = 0x0, // Pages may not be accessed.
	PROT_READ  = 0x1, // Pages may be read.
	PROT_WRITE = 0x2, // Pages may be written.
	PROT_EXEC  = 0x4  // Pages may be executed.
};

SYSCALL_DEFINE6(mmap, void *, addr, size_t, length, int, prot, int, flags, int, fd, int, offset)
{
	struct task *cur_task = task_get_current_task();
	if (!cur_task)
		return -EINVAL;

	struct vm_area *return -1;
}
