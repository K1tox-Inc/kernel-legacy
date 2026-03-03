#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <proc/exec.h>
#include <proc/section.h>
#include <proc/task.h>
#include <proc/userspace.h>
#include <utils/error.h>

void hello_kproc(void) { vga_printf("Hello from our Kernel process !"); }

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

//////////////////////////////////////////////
// Mok need to be delete when we got an elf loader + fs (ps: sa me degoute)

extern char user_cafe_start[], user_cafe_end[];
extern char user_dead_start[], user_dead_end[];
extern char kernel_task_start[], kernel_task_end[];

struct exec_fn_mok {
	const char *name;
	void       *start;
	void       *end;
	bool        is_user;
};

struct exec_fn_mok mok_registry[] = {{"cafe", user_cafe_start, user_cafe_end, true},
                                     {"dead", user_dead_start, user_dead_end, true},
                                     {"hello", kernel_task_start, kernel_task_end, false},
                                     {NULL, NULL, NULL, false}};

int exec_mok(const char *name)
{
	size_t name_len = ft_strlen(name);

	for (int i = 0; mok_registry[i].name != NULL; i++) {
		size_t reg_name_len = ft_strlen(mok_registry[i].name);
		if (reg_name_len == name_len && !ft_memcmp(mok_registry[i].name, name, name_len)) {
			size_t fn_size = (uintptr_t)mok_registry[i].end - (uintptr_t)mok_registry[i].start;
			return exec_fn(mok_registry[i].start, fn_size, (char *)mok_registry[i].name,
			               mok_registry[i].is_user);
		}
	}
	return -ENOENT;
}
