#include <drivers/vga.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <proc/task.h>
#include <proc/userspace.h>
#include <utils/error.h>
#include <utils/id_manager.h>

#define INIT_SENTINEL(name, ptr)                                                                   \
	do {                                                                                           \
		struct list_head *name = ptr;                                                              \
		name->next             = name;                                                             \
		name->prev             = name;                                                             \
	} while (0)

static t_id_manager pid_manager;

static const char *task_state_to_string(enum process_states state)
{
	if (state == NEW)
		return "NEW";
	if (state == RUNNING)
		return "RUNNING";
	if (state == WAITING)
		return "WAITING";
	if (state == ZOMBIE)
		return "ZOMBIE";
	return "UNKNOWN";
}

static void task_print_section(const char *label, const section_t *sec)
{
	vga_printf("  - %s: vaddr=%p | paddr=%p | data=%p | size=%u | map=%u | flags=0x%x\n", label,
	           (void *)sec->v_addr, (void *)sec->p_addr, (void *)sec->data_start, sec->data_size,
	           sec->mapping_size, sec->flags);
}

struct task *task_get_new(char *name, bool userspace, section_t *text, section_t *data)
{
	size_t name_len = ft_strlen(name);
	name_len        = name_len > 15 ? 15 : name_len;
	// kmalloc use slabs caches here
	char *memory_zone = kmalloc(sizeof(struct task) + name_len + 1, GFP_KERNEL | __GFP_ZERO);
	if (!memory_zone)
		return NULL;

	struct task *ret = (struct task *)memory_zone;

	ret->pid  = id_manager_alloc(&pid_manager);
	ret->name = memory_zone + sizeof(struct task);

	INIT_SENTINEL(children, &ret->children);
	INIT_SENTINEL(siblings, &ret->siblings);

	ft_memcpy(ret->name, name, name_len);
	ret->name[name_len] = 0;

	void *kstack = kmalloc(DEFAULT_STACK_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!kstack) {
		kfree(ret);
		return NULL;
	}

	ret->kernel_stack_pointer = (uintptr_t)kstack;
	ret->kernel_stack_base    = (uintptr_t)kstack + DEFAULT_STACK_SIZE;

	/*
	 * Stack Canary: Replaces hardware guard pages in higher-half linear mapping
	 * Placed at the stack lowest address to detect downward overflows
	 * MUST be verified by the scheduler during every context switch
	 */
	*(uint32_t *)(ret->kernel_stack_pointer) = STACK_CANARY_MAGIC;

	ret->esp = ret->kernel_stack_base;

	if (userspace) {
		if (!userspace_create_new(text, data, ret)) {
			kfree(kstack);
			kfree(ret);
			return NULL;
		}
		ret->ring = 3;
	} else {
		ret->cr3  = vmm_get_kernel_directory();
		ret->ring = 0;
	}

	return ret;
}

void task_print_info(const struct task *task)
{
	if (!task) {
		vga_printf("task_print_info: task pointer is NULL\n");
		return;
	}

	vga_printf("Task Info (PID %d)\n", task->pid);
	vga_printf("  - Name: %s\n", task->name ? task->name : "(null)");
	vga_printf("  - UID: %d | GID: %d\n", task->uid, task->gid);
	vga_printf("  - State: %s\n", task_state_to_string(task->state));
	vga_printf("  - Parent: %p | Real Parent: %p\n", task->parent, task->real_parent);
	vga_printf("  - Sched: next=%p | prev=%p\n", task->next, task->prev);
	vga_printf("  - CPU: esp=%p | cr3=%p\n", (void *)task->esp, (void *)task->cr3);
	vga_printf("  - Kernel Stack: base=%p | ptr=%p\n", (void *)task->kernel_stack_base,
	           (void *)task->kernel_stack_pointer);
	task_print_section("Code", &task->code_sec);
	task_print_section("Data", &task->data_sec);
	task_print_section("Stack", &task->stack_sec);
	task_print_section("Heap", &task->heap_sec);
}
