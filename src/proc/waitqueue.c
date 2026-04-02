#include <proc/lock.h>
#include <proc/task.h>
#include <proc/waitqueue.h>

void wq_prepare(struct wq_head *wq, struct task *task)
{
	lock_preempt(&task->lock);
	task->state        = TASK_WAITING;
	task->need_resched = true;
	list_add_head(&task->wq_data.node, &wq->head);
	// this call rescedule the process
	unlock_preempt(&task->lock);
}

void wq_finish(struct wq_entry *entry)
{
	lock_preempt(&entry->task->lock);
	pop_node(&entry->node);
	if (entry->handler)
		entry->handler(entry);
	entry->task->state = TASK_RUNNING;
	// sched_enqueue(entry->task);
	unlock_preempt(&entry->task->lock);
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
