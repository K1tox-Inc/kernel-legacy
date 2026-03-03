#pragma once

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

	section_t *code_sec;
	section_t *data_sec;
	section_t *stack_sec;
	section_t *heap_sec;

	/* Scheduling */
	struct task        *next; // Used for "Round Robin"
	struct task        *prev;
	enum process_states state;

	/* Signals */
	struct signal_queue signals;
	/* Info */
	char  *name;
	size_t ring;
};

extern void task_launcher(struct task *next);
extern void task_user_launcher(struct task *next);

static inline section_t *task_text(struct task *new_task) { return new_task->code_sec; }
static inline section_t *task_data(struct task *new_task) { return new_task->data_sec; }
static inline section_t *task_heap(struct task *new_task) { return new_task->heap_sec; }
static inline section_t *task_stack(struct task *new_task) { return new_task->stack_sec; }

void         task_print_info(const struct task *task);
void         task_print_stack(const struct task *task);
struct task *task_get_new(char *name, bool userspace, section_t *text, section_t *data);
void         task_init_idle(void);
