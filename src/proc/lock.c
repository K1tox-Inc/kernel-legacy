#include <proc/lock.h>
#include <proc/task.h>

volatile uint32_t preempt_count = 0;

void lock_preempt(preempt_lock *lock)
{
	lock_irq();
	preempt_count++;
	*lock = 1;
	/* On Single-Core, disabling IRQs ensures mutual exclusion
	 * For Multi-Core, need an atomic Test-And-Set loop here :
	 *
	 * while(*lock)
	 *  Atomic ops;
	 *
	 */
}

void unlock_preempt(preempt_lock *lock)
{
	*lock = 0;
	if (preempt_count)
		preempt_count--;
	if (!preempt_count) {
		struct task *cur_task = task_get_current_task();
		bool         resched  = cur_task && cur_task->need_resched;
		if (resched)
			cur_task->need_resched = false;
		// if (resched)
		// 	schedule();
		// else
		unlock_irq();
	}
}
