#include <kernel/panic.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <proc/scheduler.h>
#include <proc/section.h>
#include <proc/task.h>
#include <proc/userspace.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

enum mok_idx { IDX_CAFEBABE, IDX_DEADBEEF, MOK_SENTINEL };

#define iter_over_array(p, a)                                                                      \
	for (p = a; (uintptr_t)p - (uintptr_t)a <= sizeof(a) - sizeof(typeof(*a)); p++)

extern char user_cafe_start[], user_cafe_end[];
extern char user_dead_start[], user_dead_end[];
extern char mok_sys_get_start[], mok_sys_get_end[];

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

/*
 * Temporary execve — MOK-based (no ELF loader).
 *
 * Replaces the current process image:
 * 1. Cleans up old memory (CR3/Sections) via task_exit_cleanup.
 * 2. Loads new text and creates a fresh userspace/context.
 * 3. Switches stack and jumps to entry point via ASM.
 *
 * Does not return.
 */
SYSCALL_DEFINE1(exec_fn, int, index)
{
	if (index < 0 || index >= MOK_SENTINEL)
		return -EINVAL;

	struct exec_fn_mok fn_info = mok_registry[index];
	size_t             fn_size = (uintptr_t)fn_info.end - (uintptr_t)fn_info.start;

	struct section text;
	if (!section_init_from_buffer(&text, 0, fn_info.start, fn_size, 0))
		return -EFAULT;

	struct task *cur = task_get_current_task();
	paging_reload_cr3(vmm_get_kernel_directory());
	task_exit_cleanup(cur);

	ft_memcpy(cur->text_sec, &text, sizeof(struct section));

	if (fn_info.is_user) {
		if (!userspace_create_new(cur))
			kpanic("Error: execve fail to create a userspace clean");
	} else
		cur->cr3 = vmm_get_kernel_directory();

	size_t name_len = ft_strlen(fn_info.name);
	name_len        = name_len > 15 ? 15 : name_len;
	ft_memcpy(cur->name, fn_info.name, name_len);
	cur->name[name_len] = 0;

	task_craft_context(cur, fn_info.is_user, cur->text_sec->v_addr);
	paging_reload_cr3(cur->cr3);

	__asm__ volatile("mov %0, %%esp\n\t"
	                 "pop %%edi\n\t"
	                 "pop %%esi\n\t"
	                 "pop %%ebx\n\t"
	                 "pop %%ebp\n\t"
	                 "ret" ::"r"(cur->esp));

	kpanic("execve: reached unreachable code");
	return -EUNREACH;
}
