#pragma once

#include <list.h>
#include <proc/lock.h>
#include <proc/section.h>
#include <proc/signal.h>
#include <proc/waitqueue.h>
#include <types.h>

#define STACK_CANARY_MAGIC 0xABADBABE
#define PID_MAX            32768

enum process_states { TASK_NEW, TASK_RUNNING, TASK_WAITING, TASK_ZOMBIE };

struct task {
	pid_t pid;

	/* Owner */
	uid_t uid;
	gid_t gid;

	/* Hierarchy */
	struct task     *real_parent;
	struct task     *parent;
	struct list_head children;
	struct list_head siblings;

	/* Context */
	uintptr_t esp;
	/* Phy addr */
	uintptr_t cr3;

	uintptr_t kernel_stack_pointer;
	uintptr_t kernel_stack_base; // <------- DO NOT REORDER before this line (see context.s)

	// Sections
	struct section *text_sec;
	struct section *data_sec;
	struct section *stack_sec;
	struct section *heap_sec;

	/* Signals */
	struct signal_queue signals;

	/* Info */
	char               *name;
	size_t              ring;
	preempt_lock        lock;
	bool                need_resched;
	uint32_t            exit_code;
	enum process_states state;
	struct list_head    info_node;

	/* Scheduling */
	struct list_head sched_node; // Used for "Round Robin"
	struct wq_entry  wq_data;
	struct wq_head   child_wq;
	uint32_t         quantum_remaining;
};

static inline struct section *task_text(struct task *new_task) { return new_task->text_sec; }
static inline struct section *task_data(struct task *new_task) { return new_task->data_sec; }
static inline struct section *task_heap(struct task *new_task) { return new_task->heap_sec; }
static inline struct section *task_stack(struct task *new_task) { return new_task->stack_sec; }
static inline bool            task_is_sleeping(struct task *task)
{
	return task->state == TASK_WAITING && task->wq_data.head != NULL;
}

static inline bool task_stack_overflow(struct task *task)
{
	return (*(uint32_t *)(task->kernel_stack_pointer) != STACK_CANARY_MAGIC);
}

static inline bool task_has_child_pid(struct task *parent, pid_t child_pid)
{
	struct task *child;
	list_for_each_entry(child, &parent->children, siblings)
	{
		if (child->pid == child_pid)
			return true;
	}
	return false;
}

void         task_print_info(const struct task *task);
void         task_print_stack(const struct task *task);
void         task_append_child(struct task *parent, struct task *child);
void         task_init_process(void);
void         task_set_current_task(struct task *src);
void         task_exit_cleanup(struct task *task);
void         task_release(struct task *task);
void         task_ps(void);
void         task_craft_context(struct task *task, bool userspace, uintptr_t entry);
void         __task_reparent_children(struct task *parent);
struct task *task_get_current_task(void);
struct task *task_get_idle(void);
struct task *task_get_new(const char *name, bool userspace, struct section *text,
                          struct section *data);
