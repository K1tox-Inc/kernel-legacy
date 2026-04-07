#include <memory/usercopy.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/waitqueue.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

static struct task *find_zombie_child(struct task *parent, pid_t pid)
{
	struct task *child;
	list_for_each_entry(child, &parent->children, siblings)
	{
		if (child->state != TASK_ZOMBIE)
			continue;
		if (pid == -1 || child->pid == pid)
			return child;
	}
	return NULL;
}

SYSCALL_DEFINE3(waitpid, pid_t, pid, int *, status, int, options)
{
	// Not handled actualy
	(void)options;

	if (pid == 0 || pid < -1)
		return -EINVAL;

	struct task *cur_task = task_get_current_task();

	if (list_is_empty(&cur_task->children))
		return -ECHILD;
	else if (pid > 0 && !task_has_child_pid(cur_task, pid))
		return -ECHILD;

	while (true) {
		struct task *zombie = find_zombie_child(cur_task, pid);
		if (zombie) {
			pid_t ret = zombie->pid;
			if (status)
				if (copy_to_user(status, &zombie->exit_code, sizeof(int)))
					return -EFAULT;
			task_release(zombie);
			return ret;
		}

		wq_prepare(&cur_task->child_wq, cur_task);
		schedule();
	}
}
