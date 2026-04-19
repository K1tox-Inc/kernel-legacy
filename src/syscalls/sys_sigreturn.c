#include <arch/x86.h>
#include <drivers/vga.h>
#include <memory/usercopy.h>
#include <proc/signal.h>
#include <proc/task.h>
#include <syscalls/ksyscalls.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE0(sigreturn)
{

	struct task *cur = task_get_current_task();
	if (!cur)
		return -EINVAL;

	struct trap_frame *tf =
	    (struct trap_frame *)(cur->kernel_stack_base - sizeof(struct trap_frame));

	struct sigframe *old_frame = container_of((void *)tf->user_esp, struct sigframe, sig_num);
	if (copy_from_user(tf, &old_frame->tf_backup, sizeof(struct trap_frame)))
		sys_exit(-EFAULT);

	tf->user_ss = USER_DS;
	tf->eflags  = (tf->eflags & 0x00000DD5) | 0x202;
	tf->cs      = USER_CS;
	tf->regs.ds = USER_DS;
	tf->regs.es = USER_DS;
	tf->regs.fs = USER_DS;
	tf->regs.gs = USER_DS;

	return (long)tf->regs.eax;
}
