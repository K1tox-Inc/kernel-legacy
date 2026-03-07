#include "drivers/vga.h"
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <proc/task.h>
#include <utils/error.h>

long __do_sys_fork(void)
{
	struct task *new     = kmalloc(sizeof(struct task), __GFP_KERNEL),
	            *current = task_get_current_task();

	ft_memcpy(new, current, sizeof(struct task));

	new->pid = 0x42; /* id_manager_alloc(pid_manager);
	                  * Cannot retrieve `pid_manager`.
	                  * Should I define it as global or api to get it?
	                  */

	if (new->pid == -1)
		goto free_task;

	// TODO: Append to current->childrens

	new->parent      = current;
	new->real_parent = current;

	vga_printf("Forked process => parent pid: %ul, child pid: %ul\n", current->pid, new->pid);

	return new->pid;

free_task:
	kfree(new);

	return -EINVAL;
}
