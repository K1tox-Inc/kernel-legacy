// ============================================================================
// INCLUDES
// ============================================================================

#include <arch/x86.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/userspace.h>
#include <proc/waitqueue.h>
#include <types.h>
#include <utils/error.h>
#include <utils/id_manager.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

static struct id_manager *pid_manager  = NULL;
static struct task       *idle_task    = NULL;
struct task              *current_task = NULL;

extern char         kitoxD_start[], kitoxD_end[];
static struct task *kitoxD_task = NULL;

__attribute__((constructor)) static void init_pid_manager(void)
{
	pid_manager = id_manager_create(PID_MAX);
}

extern void interrupt_exit(void);

// ============================================================================
// INTERNAL APIs
// ============================================================================

static const char *task_state_to_string(enum process_states state)
{
	if (state == TASK_NEW)
		return "NEW";
	if (state == TASK_RUNNING)
		return "RUNNING";
	if (state == TASK_WAITING)
		return "WAITING";
	if (state == TASK_ZOMBIE)
		return "ZOMBIE";
	return "UNKNOWN";
}

static void task_print_section(const char *label, const struct section *sec)
{
	if (!sec) {
		vga_printf("  - %s: (null)\n", label);
		return;
	}

	vga_printf("  - %s: vaddr=%p | paddr=%p | data=%p | size=%u | map=%u | flags=0x%x\n", label,
	           sec->v_addr, sec->p_addr, sec->data_start, sec->data_size, sec->mapping_size,
	           sec->flags);
}

static void cpu_idle_loop(void)
{
	while (true)
		__asm__ volatile("hlt");
}

static void task_init_idle(void)
{
	idle_task = task_get_new("Idle", false, NULL, NULL);
	if (!idle_task)
		kpanic("Failed to init Idle\n");

	task_craft_context(idle_task, false, (uintptr_t)cpu_idle_loop);
	idle_task->state = TASK_NEW;
	current_task     = idle_task;
}

static void task_init_kitoxD(void)
{
	struct section text;
	size_t         fn_size = (uintptr_t)kitoxD_end - (uintptr_t)kitoxD_start;

	if (!section_init_from_buffer(&text, 0, kitoxD_start, fn_size, 0))
		kpanic("Failed to init kitoxD sections\n");

	kitoxD_task = task_get_new("kitoxD", true, &text, NULL);
	if (!kitoxD_task)
		kpanic("Failed to init kitoxD\n");

	kitoxD_task->state = TASK_NEW;
	task_craft_context(kitoxD_task, true, kitoxD_task->text_sec->v_addr);
	sched_enqueue(kitoxD_task);
}

// ============================================================================
// EXTERNAL APIs
// ============================================================================

struct task *task_get_current_task(void) { return current_task; }
struct task *task_get_kitoxD(void) { return kitoxD_task; }
struct task *task_get_idle(void) { return idle_task; }

void task_set_current_task(struct task *src) { current_task = src; }

void task_append_child(struct task *parent, struct task *child)
{
	list_add_head(&child->siblings, &parent->children);

	child->parent      = parent;
	child->real_parent = parent;
}

// Text and data are used as templates; this function allocates its own internal sections
// After return, the caller must free the input text and data if they were heap-allocated
struct task *task_get_new(const char *name, bool userspace, struct section *text,
                          struct section *data)
{
	if (!name)
		return NULL;

	size_t name_len = ft_strlen(name);
	name_len        = name_len > 15 ? 15 : name_len;

	// Ensure PID manager is initialized before allocating a PID
	if (!pid_manager)
		return NULL;

	// `kmalloc` use slabs caches here
	char *memory_zone =
	    kmalloc(sizeof(struct task) + 16 + (sizeof(struct section) * 4), GFP_KERNEL | __GFP_ZERO);
	if (!memory_zone)
		return NULL;

	struct task *ret = (struct task *)memory_zone;

	ret->text_sec  = (struct section *)(ret + 1);
	ret->data_sec  = ret->text_sec + 1;
	ret->stack_sec = ret->data_sec + 1;
	ret->heap_sec  = ret->stack_sec + 1;

	if (text)
		ft_memcpy(ret->text_sec, text, sizeof(struct section));

	if (data)
		ft_memcpy(ret->data_sec, data, sizeof(struct section));

	ret->pid = id_manager_alloc(pid_manager);
	if (ret->pid == -1)
		goto free_task;

	INIT_SENTINEL(&ret->children);
	INIT_SENTINEL(&ret->siblings);

	// `kmalloc` use buddy allocator here
	void *kstack = kmalloc(DEFAULT_STACK_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!kstack)
		goto free_pid;

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
		if (!userspace_create_new(ret))
			goto free_kstack;
		ret->ring = 3;
	}

	else {
		ret->cr3  = vmm_get_kernel_directory();
		ret->ring = 0;
	}

	ret->state = TASK_NEW;

	ret->name = (char *)(ret->heap_sec + 1);
	ft_memcpy(ret->name, name, name_len);
	ret->name[name_len] = 0;

	wq_entry_init(&ret->wq_data, ret, TASK_INTERRUPTIBLE);
	wq_init(&ret->child_wq);

	INIT_SENTINEL(&ret->sched_node);
	/*
	 * All these fields are zeroed by `kmalloc` with `__GFP_ZERO`
	 * and must be initialized by the caller if needed (like `fork`) :
	 *
	 *  uid_t uid;
	 *  gid_t gid;
	 *  preempt_lock     lock;
	 *  bool			 need_resched
	 *  struct task		*real_parent;
	 *  struct task		*parent;
	 *  struct list_head sched_node; // Used for scheduler run-queue linkage
	 *	uint32_t         exit_code;
	 *
	 */

	return ret;

free_kstack:
	kfree(kstack);
free_pid:
	id_manager_free(pid_manager, ret->pid);
free_task:
	kfree(ret);
	return NULL;
}

void __task_reparent_children(struct task *parent)
{
	if (!kitoxD_task || parent == kitoxD_task)
		return;

	struct task *child;

	while (!list_is_empty(&parent->children)) {
		child              = list_first_entry(&parent->children, struct task, siblings);
		child->parent      = kitoxD_task;
		child->real_parent = kitoxD_task;
		pop_node(&child->siblings);
		list_add_head(&child->siblings, &kitoxD_task->children);
	}
}

void task_exit_cleanup(struct task *task)
{

	struct section *text = task_text(task);
	if (text && text->p_addr)
		buddy_free_block((void *)text->p_addr);

	struct section *data = task_data(task);
	if (data && data->p_addr)
		buddy_free_block((void *)data->p_addr);

	struct section *stack = task_stack(task);
	if (stack && stack->p_addr)
		buddy_free_block((void *)stack->p_addr);

	// actually no heap must be implemented later
	// struct section *heap = task_heap(task);
	// if (heap && heap->p_addr)
	// 	buddy_free_block((void *)heap->p_addr);
	if (task->ring)
		vmm_destroy_user_pd(task->cr3);
	// close all fds in futur
}

void task_release(struct task *task)
{
	pop_node(&task->siblings);
	id_manager_free(pid_manager, task->pid);
	kfree((void *)task->kernel_stack_pointer);
	kfree(task);
}

void task_craft_context(struct task *task, bool userspace, uintptr_t entry)
{
	uint32_t *kstack = (uint32_t *)task->kernel_stack_base;

	if (userspace) {
		struct trap_frame *tf =
		    (struct trap_frame *)(task->kernel_stack_base - sizeof(struct trap_frame));
		ft_bzero(tf, sizeof(struct trap_frame));

		tf->user_ss  = USER_DS;
		tf->user_esp = task->stack_sec->v_addr + task->stack_sec->mapping_size;
		tf->eflags   = EFLAGS_USER_DEFAULT;
		tf->cs       = USER_CS;
		tf->eip      = entry;
		tf->regs.ds  = USER_DS;
		tf->regs.es  = USER_DS;
		tf->regs.fs  = USER_DS;
		tf->regs.gs  = USER_DS;

		kstack      = (uint32_t *)(task->kernel_stack_base - sizeof(struct trap_frame));
		*(--kstack) = (uint32_t)interrupt_exit;
	} else {
		*(--kstack) = entry;
	}

	*(--kstack) = 0; // ebp
	*(--kstack) = 0; // ebx
	*(--kstack) = 0; // esi
	*(--kstack) = 0; // edi

	task->esp = (uintptr_t)kstack;
}

void task_init_process(void)
{
	task_init_idle();
	task_init_kitoxD();
}

// ============================================================================
// DEBUG APIs
// ============================================================================

void task_print_info(const struct task *task)
{
	if (!task) {
		vga_printf("task_print_info: task pointer is NULL\n");
		return;
	}

	vga_printf("Task Info (PID %d)\n", task->pid);
	vga_printf("  - Name: %s\n", task->name);
	vga_printf("  - UID: %d | GID: %d\n", task->uid, task->gid);
	vga_printf("  - State: %s\n", task_state_to_string(task->state));
	vga_printf("  - Parent: %p | Real Parent: %p\n", task->parent, task->real_parent);
	vga_printf("  - Sched: next=%p | prev=%p\n", (void *)task->sched_node.next,
	           (void *)task->sched_node.prev);
	vga_printf("  - CPU: esp=%p | cr3=%p\n", (void *)task->esp, (void *)task->cr3);
	vga_printf("  - Kernel Stack: base=%p | ptr=%p\n", (void *)task->kernel_stack_base,
	           (void *)task->kernel_stack_pointer);

	task_print_section("Text", task->text_sec);
	task_print_section("Data", task->data_sec);
	task_print_section("Stack", task->stack_sec);
	task_print_section("Heap", task->heap_sec);
}

void task_print_stack(const struct task *task)
{
	if (!task) {
		vga_printf("task_print_stack: task pointer is NULL\n");
		return;
	}

	uintptr_t esp  = task->esp;
	uintptr_t base = task->kernel_stack_base;

	vga_printf("=== Stack dump for task '%s' (PID %d) ===\n", task->name, task->pid);
	vga_printf("  esp  = %p\n", (void *)esp);
	vga_printf("  base = %p  (top of stack)\n", (void *)base);

	if (!esp || !base || esp >= base) {
		vga_printf("  (stack is empty or invalid: esp >= base)\n");
		return;
	}

	size_t slot_count = (base - esp) / sizeof(uint32_t);
	vga_printf("  %u bytes used (%u dwords)\n", (unsigned)(base - esp), (unsigned)slot_count);
	vga_printf("  %s | %s\n", "ADDRESS", "VALUE");
	vga_printf("  -------------|-------------\n");

	uint32_t *ptr = (uint32_t *)esp;
	uint32_t *end = (uint32_t *)base;

	for (; ptr < end; ptr++) {
		vga_printf("  %p | 0x%x\n", (void *)ptr, *ptr);
	}

	vga_printf("=== End of stack dump ===\n");
}
