#include <arch/x86.h>
#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <proc/exec.h>
#include <proc/section.h>
#include <proc/userspace.h>
#include <utils/error.h>

static void flush_registers(uint32_t **kstack, size_t it)
{
	for (size_t i = 0; i < it; i++)
		*(--(*kstack)) = 0;
}

void hello_kproc(void) { vga_printf("Hello from our Kernel process !"); }

// look here for futur wiki https://wiki.osdev.org/Getting_to_Ring_3
// carefull here the kstack must be fully zeroed
void exec_task(struct task *task, bool userspace)
{
	if (!task || !task->text_sec || !task->stack_sec)
		return;
	uint32_t *kstack = (uint32_t *)task->kernel_stack_base;
	uintptr_t code_start;

	if (userspace) {
		code_start          = (uintptr_t)task->text_sec->v_addr;
		uintptr_t stack_top = task->stack_sec->v_addr + task->stack_sec->mapping_size;
		// those value are the only ones restored by iret
		*(--kstack) = USER_DS;             // Stack segment
		*(--kstack) = stack_top;           // ESP
		*(--kstack) = EFLAGS_USER_DEFAULT; // Eflags
		*(--kstack) = USER_CS;             // CS
		*(--kstack) = code_start;          // EIP
		flush_registers(&kstack, 4);
	} else {
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

int exec_fn(void *fn_start, size_t fn_size, char *fn_name, bool userspace)
{
	section_t text;

	// vaddr and flags are overwritten after by userspace_get_new
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
	if (!name)
		return -EINVAL;
	size_t name_len = ft_strlen(name);
	for (int i = 0; mok_registry[i].name != NULL; i++) {
		size_t reg_name_len = ft_strlen(mok_registry[i].name);
		if (reg_name_len == name_len && !ft_memcmp(mok_registry[i].name, name, name_len)) {
			size_t fn_size = (uintptr_t)mok_registry[i].end - (uintptr_t)mok_registry[i].start;
			if (i < 2)
				vga_printf("Use Ctrl + Alt + 2 for monitor qemu + info register :)\n");
			return exec_fn(mok_registry[i].start, fn_size, (char *)mok_registry[i].name,
			               mok_registry[i].is_user);
		}
	}
	return -ENOENT;
}
