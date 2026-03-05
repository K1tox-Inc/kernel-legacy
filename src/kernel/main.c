#include <proc/exec.h>
#include <proc/task.h>

void init(void);

void kernel_main(void)
{
	init();
	exec_mok("cafe");
	task_init_idle();
}
