#include <proc/task.h>
#include <proc/waitqueue.h>

void wq_prepare(struct wq_head *wq, struct task *task)
{
	task->wq_data.head = wq;
	task->state        = TASK_WAITING;
	task->need_resched = true;
	if (list_node_is_linked(&task->sched_node))
		pop_node(&task->sched_node);
	if (list_node_is_linked(&task->wq_data.node))
		pop_node(&task->wq_data.node);
	list_add_head(&task->wq_data.node, &wq->head);
}

void wq_finish(struct wq_entry *entry)
{
	if (!list_node_is_linked(&entry->node))
		return;
	pop_node(&entry->node);
	if (entry->handler)
		entry->handler(entry);
	entry->task->state = TASK_RUNNING;
	// sched_enqueue(entry->task);
	wq_entry_init(entry, entry->task, entry->state);
}

void wq_signal_wake_up(struct task *task)
{
	if (task->wq_data.state == TASK_UNINTERRUPTIBLE)
		return;
	wq_wake_specifiq(task);
}

void wq_wake_specifiq(struct task *task) { wq_finish(&task->wq_data); }

void wq_wake_one(struct wq_head *wq)
{
	if (list_is_empty(&wq->head))
		return;
	struct wq_entry *entry = list_first_entry(&wq->head, struct wq_entry, node);
	wq_finish(entry);
}

void wq_wake_all(struct wq_head *wq)
{
	struct wq_entry *entry;

	while (!list_is_empty(&wq->head)) {
		entry = list_first_entry(&wq->head, struct wq_entry, node);
		wq_finish(entry);
	}
}
