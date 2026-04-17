#include <drivers/vga.h>
#include <proc/signal.h>
#include <proc/task.h>
#include <proc/waitqueue.h>
#include <syscalls/ksyscalls.h>
#include <utils/error.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

#define iter_over_array(p, a)                                                                      \
	for (p = a; (uintptr_t)p - (uintptr_t)a <= sizeof(a) - sizeof(typeof(*a)); p++)

#define SIG_IGN_MASK                                                                               \
	((1U << SIGCHLD) | (1U << SIGCONT) | (1U << SIGSTOP) | (1U << SIGTSTP) | (1U << SIGTTIN) |     \
	 (1U << SIGTTOU) | (1U << SIGURG) | (1U << SIGWINCH) | (1U << SIGPOLL) | (1U << SIGPWR) |      \
	 (1U << SIGSTKFLT))

static const char *default_msg[] = {
    [SIGHUP]    = "Hangup",
    [SIGINT]    = "Interrupt",
    [SIGQUIT]   = "Quit",
    [SIGILL]    = "Illegal instruction",
    [SIGTRAP]   = "Trace/breakpoint trap",
    [SIGABRT]   = "Aborted",
    [SIGBUS]    = "Bus error",
    [SIGFPE]    = "Floating point exception",
    [SIGKILL]   = "Killed",
    [SIGUSR1]   = "User defined signal 1",
    [SIGSEGV]   = "Segmentation fault",
    [SIGUSR2]   = "User defined signal 2",
    [SIGPIPE]   = "Broken pipe",
    [SIGALRM]   = "Alarm clock",
    [SIGTERM]   = "Terminated",
    [SIGSTKFLT] = "Stack fault",
    [SIGCHLD]   = "Child exited",
    [SIGCONT]   = "Continued",
    [SIGSTOP]   = "Stopped (signal)",
    [SIGTSTP]   = "Stopped (user)",
    [SIGTTIN]   = "Stopped (tty input)",
    [SIGTTOU]   = "Stopped (tty output)",
    [SIGURG]    = "Urgent I/O condition",
    [SIGXCPU]   = "CPU time limit exceeded",
    [SIGXFSZ]   = "File size limit exceeded",
    [SIGVTALRM] = "Virtual timer expired",
    [SIGPROF]   = "Profiling timer expired",
    [SIGWINCH]  = "Window changed",
    [SIGPOLL]   = "I/O possible",
    [SIGPWR]    = "Power failure",
    [SIGSYS]    = "Bad system call",
};

// ============================================================================
// INTERNAL APIs
// ============================================================================

static void signal_default_handler(int sig)
{
	vga_printf("%s\n", default_msg[sig]);
	sys_exit(-1);
}

static void signal_ignore_handler(int sig) { (void)sig; }

// ============================================================================
// EXTERNAL APIs
// ============================================================================

bool signal_is_valid(enum signals sig) { return (sig >= 0 && sig < SIG_Sentinel); }

bool signal_check_perm(struct task *target)
{
	struct task *cur = task_get_current_task();

	if (!cur || !target)
		return false;
	else if (cur->uid == 0)
		return true;
	else if (cur->uid == target->uid)
		return true;

	return false;
}

void signal_send(enum signals sig, struct task *dst)
{
	if (!(dst && sig > 0 && sig < SIG_Sentinel))
		return;
	dst->signals_map |= (1U << sig);
	// TASK_UNINTERRUPTIBLE state check is handled inside wq_signal_wake_up
	wq_signal_wake_up(dst);
}

bool signal_is_set(enum signals sig, struct task *dst)
{
	if (dst && sig < SIG_Sentinel) {
		return (dst->signals_map >> sig) & 1;
	}
	return false;
}

void signal_reset(struct task *dst)
{
	if (dst)
		dst->signals_map = 0;
}

void signal_dequeue(enum signals sig, struct task *dst)
{
	if (dst && sig > 0 && sig < SIG_Sentinel)
		dst->signals_map &= ~(1U << sig);
}

int signal_dequeue_yield(struct task *task)
{
	if (!task)
		return -ESRCH;
	for (int i = 1; i < SIG_Sentinel; i++) {
		if (!((task->signals_map >> i) & 1U))
			continue;
		signal_dequeue(i, task);
		return i;
	}
	return 0;
}

// ============================================================================
// Handlers
// ============================================================================

void signal_init_default_handlers(struct task *task)
{
	for (int i = 0; i < SIG_Sentinel; i++) {
		if ((SIG_IGN_MASK >> i) & 1U)
			task->sig_handlers[i] = signal_ignore_handler;
		else
			task->sig_handlers[i] = signal_default_handler;
	}
}

void signal_call_curtask_handlers(void)
{
	struct task *cur = task_get_current_task();
	if (!cur)
		return;
	for (int i = 1; i < SIG_Sentinel; i++) {
		if (signal_is_set(i, cur)) {
			signal_dequeue(i, cur);
			cur->sig_handlers[i](i);
		}
	}
}
