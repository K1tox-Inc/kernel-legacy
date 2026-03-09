#include <syscall/syscall.h>
#include <utils/error.h>

void do_syscall(struct trap_frame *tf)
{
	if (!tf) {
		tf->regs.eax = -ENOSYS;
		return;
	}
	long           sys_id  = tf->regs.eax;
	syscallHandler handler = syscall_table[sys_id];

	if (sys_id > MAX_SYSCALL || !handler) {
		tf->regs.eax = -ENOSYS;
		return;
	}
	tf->regs.eax = handler(tf);
}
