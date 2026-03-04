#include <arch/x86.h>
#include <proc/exec.h>
#include <proc/task.h>

void kernel_main(void)
{
	x86_init();
	exec_mok("cafe");
	task_init_idle();
}
