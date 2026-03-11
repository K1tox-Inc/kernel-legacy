#include <drivers/vga.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vmm.h>
#include <proc/task.h>
#include <utils/error.h>

#ifndef NDEBUG
# define log(msg, ...)      vga_printf("[" __FILE__ ":%i]: " msg "\n", __LINE__, ##__VA_ARGS__)
# define dbg(variable_name) // log(#variable_name " = $%X", variable_name)
#else
# define log(msg, ...)
# define dbg(variable_name)
#endif

long do_fork(void)
{
	log("Entered in fork syscall...");

	struct task *current = task_get_current_task(), *new;

	log("Creating new task from parent...");

	new = task_get_new(current->name, current->ring == 3, current->text_sec, current->data_sec);
	if (!new)
		return -ENOMEM;

	log("New task created!");

	task_append_child(current, new);

	log("Parent's child list now points to new task.");

	uint32_t *current_pde = PHYS_TO_VIRT_LINEAR((uint32_t *)current->cr3);
	uint32_t *new_pde     = PHYS_TO_VIRT_LINEAR((uint32_t *)new->cr3);

	log("Copying page descriptors and page tables...");

	dbg(current_pde);
	dbg(new_pde);

	for (int i = 0; i < 1024; i++, current_pde++, new_pde++) {
		*new_pde = (*new_pde & ~0xFFF) | (*current_pde & 0xFFF);

		dbg(*new_pde);

		uint32_t *current_pte = PHYS_TO_VIRT_LINEAR((uint32_t *)(*current_pde & ~0xFFF));
		uint32_t *new_pte     = PHYS_TO_VIRT_LINEAR((uint32_t *)(*new_pde & ~0xFFF));

		for (int j = 0; j < 1024; j++, current_pte++, new_pte++) {
			*new_pte = *current_pte;
			dbg(*new_pte);
		}
	}

	log("Forked process => parent pid: %ul, child pid: %ul", current->pid, new->pid);

	return new->pid;
}
