#include <syscalls/syscalls.h>
#include <utils/error.h>

void do_syscall(struct trap_frame *tf)
{
	if (!tf)
		return;

	uint32_t sys_id = tf->regs.eax;
	if (sys_id > MAX_SYSCALL)
		goto bad;

	syscallHandler handler = syscall_table[sys_id];
	if (!handler)
		goto bad;

	tf->regs.eax = handler(tf->regs.ebx, tf->regs.ecx, tf->regs.edx, tf->regs.esi, tf->regs.edi);
	return;

bad:
	tf->regs.eax = -ENOSYS;
	return;
}
