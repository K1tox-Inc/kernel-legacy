#pragma once

#include <list.h>
#include <proc/section.h>
#include <proc/signal.h>
#include <types.h>

#define STACK_CANARY_MAGIC 0xCAFEBABE
#define PID_MAX            32768

enum process_states { TASK_NEW, TASK_RUNNING, TASK_WAITING, TASK_ZOMBIE };

struct task {
	pid_t pid;

	/* Owner */
	uid_t uid;
	gid_t gid;

	/* Hierarchy */
	struct task *real_parent;
	struct task *parent;

	struct list_head children;
	struct list_head siblings;

	/* Context */
	uintptr_t esp;
	uintptr_t cr3;

	uintptr_t kernel_stack_pointer;
	uintptr_t kernel_stack_base;

	struct section *text_sec;
	struct section *data_sec;
	struct section *stack_sec;
	struct section *heap_sec;

	/* Scheduling */
	struct task        *next, *prev; // Used for "Round Robin"
	enum process_states state;

	/* Signals */
	struct signal_queue signals;

	/* Info */
	char  *name;
	size_t ring;
};

extern void task_launcher(struct task *next);
extern void task_user_launcher(struct task *next);

static inline struct section *task_text(struct task *new_task) { return new_task->text_sec; }
static inline struct section *task_data(struct task *new_task) { return new_task->data_sec; }
static inline struct section *task_heap(struct task *new_task) { return new_task->heap_sec; }
static inline struct section *task_stack(struct task *new_task) { return new_task->stack_sec; }

void               task_print_info(const struct task *task);
void               task_print_stack(const struct task *task);
void               task_append_child(struct task *parent, struct task *child);
void               task_init_idle(void);
void               task_set_current_task(struct task *src);
const struct task *task_get_current_task(void);

struct task *task_get_new(const char *name, bool userspace, struct section *text,
                          struct section *data);
