#include <drivers/vga.h>
#include <libk.h>
#include <memory/usercopy.h>
#include <memory/vmm.h>
#include <proc/task.h>

static unsigned long usercopy(void *kernel_buf, void *user_buf, unsigned long n,
                              bool copy_into_kbuf)
{
	if (!access_ok(user_buf, n))
		return n;

	uint32_t *pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(task_get_current_task()->cr3);

	uint32_t flags = PDE_PRESENT_BIT | PDE_US_BIT;
	if (!copy_into_kbuf)
		flags |= PDE_RW_BIT;

	int ret = vmm_verify_range_flags(pd_virt, user_buf, n, flags, flags);
	if (ret)
		return ret;

	if (copy_into_kbuf)
		ft_memcpy(kernel_buf, user_buf, n);
	else
		ft_memcpy(user_buf, kernel_buf, n);
	return 0;
}

unsigned long copy_from_user(void *to, const void *from, unsigned long n)
{
	return usercopy(to, (void *)from, n, true);
}

unsigned long copy_to_user(void *to, const void *from, unsigned long n)
{
	return usercopy((void *)from, to, n, false);
}
