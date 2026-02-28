#include <arch/x86.h>
#include <proc/task.h>

void kernel_main(void)
{
	x86_init();
	task_init_idle();
}
