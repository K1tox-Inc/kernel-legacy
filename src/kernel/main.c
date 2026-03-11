#include <proc/task.h>

void init(void);

void kernel_main(void)
{
	init();
	task_init_idle();
}
