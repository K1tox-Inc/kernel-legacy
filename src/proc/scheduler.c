#include <kernel/panic.h>
#include <list.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <utils/error.h>

static struct list_head ready_queue = LIST_HEAD_INIT(ready_queue);

static struct task *pick_next(void)
{
	if (list_is_empty(&ready_queue))
		return NULL;
	struct task *next = list_first_entry(&ready_queue, struct task, sched_node);
	pop_node(&next->sched_node);
	return next;
}

int sched_enqueue(struct task *task)
{
	if (!task)
		return -EINVAL;
	else if (list_node_is_linked(&task->sched_node))
		return -EINVAL;
	task->state = TASK_RUNNING;
	list_add_tail(&task->sched_node, &ready_queue);
	return SUCCESS;
}

void schedule(void)
{
	struct task *idle_task = task_get_idle();
	struct task *cur_task  = task_get_current_task();
	if (!idle_task)
		kpanic("Error: idle not init.");
	else if (cur_task == NULL)
		kpanic("Error: current task is NULL.");
	else if (task_stack_overflow(cur_task))
		kpanic("Stack overflow: (pid=%d name=%s)", cur_task->pid, cur_task->name);
	else if (cur_task->state == TASK_RUNNING && cur_task != idle_task)
		sched_enqueue(cur_task);

	struct task *next_task = pick_next();
	if (next_task && next_task->state != TASK_RUNNING)
		kpanic("Scheduler: non-RUNNING task in ready queue (pid=%d name=%s)", next_task->pid,
		       next_task->name);

	if (!next_task) {
		next_task = idle_task;
		if (cur_task == idle_task)
			return;
	}
	if (task_stack_overflow(next_task))
		kpanic("Stack overflow: (pid=%d name=%s)", next_task->pid, next_task->name);
	task_set_current_task(next_task);
	// need to check the implementation when FCFS is implemented
	// switch_context(next_task);
	kpanic("Error: cannot use schedule actually switch context not implemented.");
}
