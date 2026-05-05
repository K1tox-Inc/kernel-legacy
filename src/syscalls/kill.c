#include <proc/signal.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE2(kill, pid_t, pid, int, sig)
{
	struct task *task_target = task_find_by_pid(pid);

	if (!task_target)
		return -ESRCH;
	else if (!signal_is_valid(sig))
		return -EINVAL;
	else if (!signal_check_perm(task_target))
		return -EPERM;

	signal_send(sig, task_target);
	return SUCCESS;
}
