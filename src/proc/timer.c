#include <arch/acpi.h>
#include <arch/trap_frame.h>
#include <arch/x86.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/timer.h>

/*more info here http://www.osdever.net/bkerndev/Docs/pit.htm*/

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
	struct task *idle    = task_get_idle();
	struct task *current = task_get_current_task();
	if (--current->quantum_remaining || current == idle)
		return;
	schedule();
}
