#include <memory/vma.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE2(munmap, void *, addr, size_t, length)
{
	if (!addr || !length)
		return -EINVAL;

	struct task    *cur  = task_get_current_task();
	struct vm_area *area = vma_find_by_start(addr, &cur->vma_areas);
	if (!area || (area->state != VM_AREA_ALLOCATED && area->state != VM_AREA_LAZY))
		return -EINVAL;

	vma_destroy_area(&cur->vma_areas, area, cur->cr3);
	return 0;
}
