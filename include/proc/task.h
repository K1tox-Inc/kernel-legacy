#pragma once

#include <proc/section.h>
#include <proc/signal.h>
#include <proc/userspace.h>
#include <types.h>

enum process_states { NEW, RUNNING, WAITING, ZOMBIE };

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

	section_t code_sec;
	section_t data_sec;
	section_t stack_sec;
	section_t heap_sec;

	/* Scheduling */
	struct task        *next; // Used for "Round Robin"
	struct task        *prev;
	enum process_states state;

	/* Signals */
	struct signal_queue signals;
};
