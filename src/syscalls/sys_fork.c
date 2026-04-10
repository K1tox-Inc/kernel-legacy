#include <arch/trap_frame.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vmm.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>
#include <utils/kmacro.h>

extern void interrupt_exit(void);

SYSCALL_DEFINE0(fork)
{
	struct trap_frame *parent_tf = NULL, *child_tf;
	struct task       *current   = task_get_current_task(), *new;
	uint32_t          *current_pde, *new_pde, *current_pte, *new_pte, *kstack;

	__asm__ volatile("mov (%%ebp), %%eax\n\t"
	                 "mov 8(%%eax), %0"
	                 : "=r"(parent_tf)
	                 :
	                 : "eax");

	if (!parent_tf)
		return -EINVAL;

	new = task_get_new(current->name, current->ring == 3, current->text_sec, current->data_sec);
	if (!new)
		return -ENOMEM;

	task_append_child(current, new);

	current_pde = PHYS_TO_VIRT_LINEAR(current->cr3);
	new_pde     = PHYS_TO_VIRT_LINEAR(new->cr3);

	for (int i = 0; i < 768; i++, current_pde++, new_pde++) {
		if (FLAG_IS_SET(*current_pde, PDE_PRESENT_BIT))
			continue;

		*new_pde = GET_ENTRY_ADDR(*new_pde) | (*current_pde & ENTRY_FLAGS_MASK);

		current_pte = PHYS_TO_VIRT_LINEAR(GET_ENTRY_ADDR(*current_pde));
		new_pte     = PHYS_TO_VIRT_LINEAR(GET_ENTRY_ADDR(*new_pde));

		for (int j = 0; j < 1024; j++, current_pte++, new_pte++) {
			if (FLAG_IS_SET(*current_pte, PTE_PRESENT_BIT))
				continue;

			FLAG_SET(*current_pte, PTE_RW_BIT);
			*new_pte = *current_pte;
		}
	}

	child_tf = (struct trap_frame *)(new->kernel_stack_base - sizeof(struct trap_frame));
	ft_memcpy(child_tf, parent_tf, sizeof(struct trap_frame));
	child_tf->regs.eax = 0;

	kstack      = (uint32_t *)(new->kernel_stack_base - sizeof(struct trap_frame));
	*(--kstack) = (uint32_t)interrupt_exit;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;

	new->esp   = (uintptr_t)kstack;
	new->state = TASK_RUNNING;
	sched_enqueue(new);

	return new->pid;
}
