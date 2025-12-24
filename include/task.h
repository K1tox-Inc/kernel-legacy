#ifndef TASK_H
#define TASK_H

#include "signal.h"
#include "types.h"

enum ProcessStates { NEW, RUNNING, WAITING, ZOMBIE };

struct task {
	pid_t pid;

	/* Owner */
	uid_t uid;
	gid_t gid;

	/* Hierarchy */
	struct task *real_parent;
	struct task *parent;

	struct list_head childrens;
	struct list_head siblings;

	/* Context */
	uintptr_t esp;
	uintptr_t cr3;

	uintptr_t kernel_stack_pointer;
	uintptr_t user_stack_pointer;
	uintptr_t heap_pointer;

	/* Scheduling */
	struct task *next; // Used for "Round Robin"
	struct task *prev;

	/* Signals */
	struct signal_queue signals;
};

#endif
