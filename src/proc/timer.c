#include <arch/acpi.h>
#include <arch/trap_frame.h>
#include <arch/x86.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/timer.h>
#include <proc/waitqueue.h>

/*more info here http://www.osdever.net/bkerndev/Docs/pit.htm*/

DECLARE_WQ_HEAD(sleep_wq);
uint32_t global_ticks;

static void task_wake_up_sleepers(void)
{
	struct list_head *pos, *tmp;
	list_for_each_safe(pos, tmp, &sleep_wq.head)
	{
		struct wq_entry *entry = list_entry(pos, struct wq_entry, node);
		if (entry->task->tick_to_wake <= global_ticks)
			wq_finish(entry);
	}
}

uint32_t timer_get_global_ticks(void) { return global_ticks; }

void timer_set_cycle(int hz)
{
	int divisor = PIT_BASE_FREQUENCY / hz; /* Calculate our divisor      */
	outb(0x43, 0x36);                      /* Set our command byte 0x36  */
	outb(0x40, divisor & 0xFF);            /* Set low byte of divisor    */
	outb(0x40, divisor >> 8);              /* Set high byte of divisor   */
}

void timer_handle(struct trap_frame *frame)
{
	(void)frame;
	global_ticks++;

	task_wake_up_sleepers();

	struct task *idle    = task_get_idle();
	struct task *current = task_get_current_task();

	if (current != idle && --current->quantum_remaining > 0)
		return;
	schedule();
}

void timer_ksleep(uint32_t seconds)
{
	struct task *cur  = task_get_current_task();
	cur->tick_to_wake = global_ticks + SECONDS_TO_TICKS(seconds);
	wq_prepare(&sleep_wq, cur);
	schedule();
	cur->tick_to_wake = 0;
}
