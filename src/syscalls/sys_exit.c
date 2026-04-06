#include <kernel/panic.h>
#include <libk.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>

SYSCALL_DEFINE1(exit, int, status)
{
	struct task *cur_task = task_get_current_task();
	struct task *parent   = cur_task->real_parent;

	if (!parent)
		kpanic("Error: Trying to exit init or idle...");

	cur_task->exit_code = (status & 0xff) << 8;

	if (list_node_is_linked(&cur_task->sched_node))
		pop_node(&cur_task->sched_node);

	__task_reparent_children(cur_task);
	cur_task->state = TASK_ZOMBIE;

	wq_wake_all(&parent->child_wq);
	// send_signal to parent
	task_exit_cleanup(cur_task);
	// schedule() needs to be implemented; do not return after tearing down
	// the current task/address space.
	kpanic("sys_exit returned after task_exit_cleanup()");
	return status;
}
