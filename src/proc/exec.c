#include <arch/x86.h> // Should be removed or conditional preprocessing
#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <proc/exec.h>
#include <proc/section.h>
#include <proc/userspace.h>
#include <utils/error.h>
#include <utils/kmacro.h>

static void flush_registers(uint32_t **kstack, size_t it)
{
	for (size_t i = 0; i < it; i++)
		*(--(*kstack)) = 0;
}

// Look here for futur wiki https://wiki.osdev.org/Getting_to_Ring_3
// Carefull here the kstack must be fully zeroed
void exec_task(struct task *task, bool userspace)
{
	uint32_t *kstack;
	uintptr_t code_start;

	if (!task || !task->text_sec || !task->stack_sec)
		return;

	kstack = (uint32_t *)task->kernel_stack_base;

	if (userspace) {
		code_start          = (uintptr_t)task->text_sec->v_addr;
		uintptr_t stack_top = task->stack_sec->v_addr + task->stack_sec->mapping_size;

		// Those value are the only ones restored by iret
		*(--kstack) = USER_DS;             // Stack segment
		*(--kstack) = stack_top;           // ESP
		*(--kstack) = EFLAGS_USER_DEFAULT; // Eflags
		*(--kstack) = USER_CS;             // CS
		*(--kstack) = code_start;          // EIP

		flush_registers(&kstack, 4);
	}

	else {
		code_start  = (uintptr_t)task->text_sec->data_start;
		*(--kstack) = code_start;
		flush_registers(&kstack, 4);
	}

	task->esp = (uintptr_t)kstack;

	if (userspace)
		task_user_launcher(task);
	else
		task_launcher(task);
}

int exec_fn(void *fn_start, size_t fn_size, const char *fn_name, bool userspace)
{
	struct section text;

	// `vaddr` and `flags` are overwritten after by `userspace_get_new`
	if (!section_init_from_buffer(&text, 0, fn_start, fn_size, 0))
		return -EFAULT;

	struct task *fn_task = task_get_new(fn_name, userspace, &text, NULL);
	if (!fn_task)
		return -ENOMEM;

	exec_task(fn_task, userspace);
	return SUCCESS;
}

//////////////////////////////////////////////
// Mok need to be delete when we got an elf loader + fs (ps: sa me degoute)

#define iter_over_array(p, a)                                                                      \
	for (p = a; (uintptr_t)p - (uintptr_t)a <= sizeof(a) - sizeof(typeof(*a)); p++)

extern char user_cafe_start[], user_cafe_end[];
extern char user_dead_start[], user_dead_end[];

struct exec_fn_mok {
	const char *name;
	void       *start;
	void       *end;
	bool        is_user;
};

const struct exec_fn_mok mok_registry[] = {
    {"cafe", user_cafe_start, user_cafe_end, true},
    {"dead", user_dead_start, user_dead_end, true},
};

static inline bool ft_strequ(const char *s1, const char *s2)
{
	size_t len1 = ft_strlen(s1);
	size_t len2 = ft_strlen(s2);

	if (len1 != len2)
		return false;

	return ft_memcmp(s1, s2, len1 + 1) == 0;
}

int exec_mok(const char *name)
{
	const struct exec_fn_mok *ptr;

	if (!name)
		return -EINVAL;

	iter_over_array(ptr, mok_registry)
	{
		if (ft_strequ(ptr->name, name)) {
			log("Executing `%s` in %s space...", name, ptr->is_user ? "user" : "kernel");
			size_t fn_size = (uintptr_t)ptr->end - (uintptr_t)ptr->start;
			return dbg(exec_fn(ptr->start, fn_size, name, ptr->is_user));
		}
	}

	log("No mok named `%s` found.", name);

	return -ENOENT;
}
