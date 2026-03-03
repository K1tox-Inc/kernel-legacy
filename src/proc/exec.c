#include <memory/kmalloc.h>
#include <proc/exec.h>
#include <proc/section.h>
#include <proc/task.h>
#include <proc/userspace.h>
#include <utils/error.h>
#include <drivers/vga.h>
#include <libk.h>

void    hello_kproc(void) { vga_printf("Hello from our Kernel process !");}

int exec_fn(void *fn_start, size_t fn_size, char *fn_name, bool userspace)
{
	section_t text;

	// vaddr and flags are overwritted after by userspace_get_new
	if (!section_init_from_buffer(&text, 0, fn_start, fn_size, 0))
		return -EFAULT;

	struct task *fn_task = task_get_new(fn_name, userspace, &text, NULL);
	if (!fn_task)
		return -ENOMEM;
        uint32_t *kstack = (uint32_t *)fn_task->kernel_stack_base;


	return SUCCESS;
}

