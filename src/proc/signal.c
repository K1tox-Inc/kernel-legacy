#include <proc/task.h>
#include <signal.h>

bool signal_is_valid(enum signals sig) { return (sig >= 0 && sig < Sentinel); }

bool signal_check_perm(struct task *target)
{
	struct task *cur = task_get_current_task();

	if (!cur || !target)
		return false;
	else if (cur->uid == 0)
		return true;
    else if  (cur->uid == target->uid)
        return true;
    
    return false;
}

void signal_send(enum signals sig, struct task *dst)
{
	// Here we need to handle task state
	if (dst && sig > 0 && sig < Sentinel)
		dst->signals_map |= (1 << sig);
}

bool signal_is_set(enum signals sig, struct task *dst)
{
	if (dst && sig < Sentinel) {
		return (dst->signals_map >> sig) & 1;
	}
	return false;
}

void signal_reset(struct task *dst)
{
	if (dst)
		dst->signals_map = 0;
}
