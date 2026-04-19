#include <proc/signal.h>
#include <proc/task.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE2(signal, int, signum, sighandler_t, handler)
{
	if (!signal_is_valid(signum) || !handler)
		return -EINVAL;
	else if (signum == SIGKILL || signum == SIGSTOP)
		return -EINVAL;

	struct task *cur = task_get_current_task();

	sighandler_t old          = cur->sig_handlers[signum];
	cur->sig_handlers[signum] = handler;

	return (long)old;
}
