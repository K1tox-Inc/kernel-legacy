#include <memory/vma.h>
#include <memory/vmm.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

enum mmap_prot {
	PROT_NONE  = 0x0, // Pages may not be accessed.
	PROT_READ  = 0x1, // Pages may be read.
	PROT_WRITE = 0x2, // Pages may be written.
	PROT_EXEC  = 0x4  // Pages may be executed.
};

SYSCALL_DEFINE6(mmap, void *, addr, size_t, length, int, prot, int, flags, int, fd, int, offset)
{
	(void)fd;
	(void)offset;
	(void)flags;
	if (!length)
		return -EINVAL;

	struct task *cur       = task_get_current_task();
	uint32_t     pte_flags = PTE_PRESENT_BIT | PTE_US_BIT;
	if (prot & PROT_WRITE)
		pte_flags |= PTE_RW_BIT;

	struct vm_area *area = vma_alloc(&cur->vma_areas, cur->cr3, length, pte_flags, addr);
	if (!area)
		return -ENOMEM;

	return (long)area->start_vaddr;
}
