#include <arch/trap_frame.h>
#include <kernel/panic.h>
#include <libk.h>
#include <list.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <memory/vmm.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/assert.h>
#include <utils/error.h>
#include <utils/kmacro.h>

extern void         interrupt_exit(void);
extern struct task *task_clone(const struct task *task);

SYSCALL_DEFINE0(fork)
{
	struct task *current = task_get_current_task(), *new;
	assert(current);

	struct trap_frame *child_tf;
	uint32_t          *current_pde, *new_pde, *current_pte, *new_pte;

	new = task_clone(current);
	if (!new)
		return -ENOMEM;

	new->kernel_stack_pointer = (uintptr_t)kmalloc(DEFAULT_STACK_SIZE, __GFP_KERNEL | __GFP_ZERO);
	if (!new->kernel_stack_pointer)
		goto fail;

	new->kernel_stack_base = new->kernel_stack_pointer + DEFAULT_STACK_SIZE;

	current_pde = PHYS_TO_VIRT_LINEAR(current->cr3);
	new_pde     = PHYS_TO_VIRT_LINEAR(new->cr3);

	// Quick fix, should be really fixed further
	new->text_sec->p_addr  = 0;
	new->data_sec->p_addr  = 0;
	new->heap_sec->p_addr  = 0;
	new->stack_sec->p_addr = 0;

	for (int i = 0; i < 768; i++, current_pde++, new_pde++) {
		if (FLAG_IS_SET(*current_pde, PDE_PRESENT_BIT)) {
			uintptr_t new_pt_phys = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
			if (!new_pt_phys)
				goto fail;

			*new_pde = GET_ENTRY_ADDR(new_pt_phys) | GET_ENTRY_FLAGS(*current_pde);

			current_pte = PHYS_TO_VIRT_LINEAR(GET_ENTRY_ADDR(*current_pde));
			new_pte     = PHYS_TO_VIRT_LINEAR(GET_ENTRY_ADDR(*new_pde));

			ft_bzero(new_pte, PAGE_SIZE);

			for (int j = 0; j < 1024; j++, current_pte++, new_pte++) {
				if (FLAG_IS_SET(*current_pte, PTE_PRESENT_BIT)) {

					const uintptr_t page = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, HIGHMEM_ZONE);
					assert(page);

					void *const window = vmm_kmap(page);
					assert(window);

					assert(vmm_map_page(new->cr3, i << 22 | j << 12, page,
					                    GET_ENTRY_FLAGS(*current_pte)));

					ft_memcpy(window, (void *)(i << 22 | j << 12), PAGE_SIZE);

					vmm_kunmap();
				}
			}
		}
	}

	ft_memcpy(new_pde, current_pde, 224 * sizeof(uint32_t));

	ft_memcpy((void *)new->kernel_stack_pointer, (void *)current->kernel_stack_pointer,
	          DEFAULT_STACK_SIZE);

	assert(*(uint32_t *)(new->kernel_stack_pointer) == STACK_CANARY_MAGIC);
	assert(*(uint32_t *)(current->kernel_stack_base - 4) ==
	       *(uint32_t *)(new->kernel_stack_base - 4));

	child_tf = (struct trap_frame *)(new->kernel_stack_base - sizeof(struct trap_frame));
	assert(ft_memcmp(child_tf, (void *)current->kernel_stack_base - sizeof(struct trap_frame),
	                 sizeof(struct trap_frame)) == 0);
	child_tf->regs.eax = 0;

	task_append_child(current, new);

	new->esp   = new->kernel_stack_pointer - (current->kernel_stack_pointer - current->esp);
	new->state = TASK_RUNNING;

	sched_enqueue(new);

	uint32_t *stack = (uint32_t *)(new->kernel_stack_base - sizeof(struct trap_frame));
	*(--stack)      = (uint32_t)interrupt_exit;

	*(--stack) = 0; // ebp
	*(--stack) = 0; // ebx
	*(--stack) = 0; // esi
	*(--stack) = 0; // edi

	return new->pid;

fail:
	vmm_destroy_user_pd(new->cr3);
	task_release(new);
	return -ENOMEM;
}
