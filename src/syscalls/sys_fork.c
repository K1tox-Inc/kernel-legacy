#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vmm.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>
#include <utils/kmacro.h>

SYSCALL_DEFINE0(fork)
{
	struct task *current = task_get_current_task(), *new;

	new = task_get_new(current->name, current->ring == 3, current->text_sec, current->data_sec);
	if (!new)
		return -ENOMEM;

	task_append_child(current, new);

	uint32_t *current_pde = PHYS_TO_VIRT_LINEAR((uint32_t *)current->cr3);
	uint32_t *new_pde     = PHYS_TO_VIRT_LINEAR((uint32_t *)new->cr3);

	for (int i = 0; i < 1024; i++, current_pde++, new_pde++) {
		if ((*current_pde & PDE_PRESENT_BIT) == 0)
			continue;

		*new_pde = (*new_pde & ~0xFFF) | (*current_pde & 0xFFF);

		uint32_t *current_pte = PHYS_TO_VIRT_LINEAR((uint32_t *)(*current_pde & ~0xFFF));
		uint32_t *new_pte     = PHYS_TO_VIRT_LINEAR((uint32_t *)(*new_pde & ~0xFFF));

		for (int j = 0; j < 1024; j++, current_pte++, new_pte++) {
			if ((*current_pte & PTE_PRESENT_BIT) == 0)
				continue;

			*current_pte &= ~PTE_RW_BIT;
			*new_pte = *current_pte;
		}
	}

	log("Forked process => parent pid: %i, child pid: %i", current->pid, new->pid);

	return new->pid;
}
