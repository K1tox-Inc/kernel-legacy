#pragma once

#include <arch/trap_frame.h>
#include <types.h>

struct task;

enum signals {
	SIGHUP       = 1,  /* Hangup */
	SIGINT       = 2,  /* Interrupt (Ctrl+C) */
	SIGQUIT      = 3,  /* Quit */
	SIGILL       = 4,  /* Illegal instruction */
	SIGTRAP      = 5,  /* Trace/breakpoint trap */
	SIGABRT      = 6,  /* Aborted */
	SIGIOT       = 6,  /* Alias pour SIGABRT */
	SIGBUS       = 7,  /* Bus error */
	SIGFPE       = 8,  /* Floating point exception */
	SIGKILL      = 9,  /* Killed (force quit) */
	SIGUSR1      = 10, /* User defined signal 1 */
	SIGSEGV      = 11, /* Segmentation fault */
	SIGUSR2      = 12, /* User defined signal 2 */
	SIGPIPE      = 13, /* Broken pipe */
	SIGALRM      = 14, /* Alarm clock */
	SIGTERM      = 15, /* Terminated */
	SIGSTKFLT    = 16, /* Stack fault */
	SIGCHLD      = 17, /* Child exited */
	SIGCLD       = 17, /* Alias pour SIGCHLD */
	SIGCONT      = 18, /* Continued */
	SIGSTOP      = 19, /* Stopped (signal) */
	SIGTSTP      = 20, /* Stopped (user) */
	SIGTTIN      = 21, /* Stopped (tty input) */
	SIGTTOU      = 22, /* Stopped (tty output) */
	SIGURG       = 23, /* Urgent I/O condition */
	SIGXCPU      = 24, /* CPU time limit exceeded */
	SIGXFSZ      = 25, /* File size limit exceeded */
	SIGVTALRM    = 26, /* Virtual timer expired */
	SIGPROF      = 27, /* Profiling timer expired */
	SIGWINCH     = 28, /* Window changed */
	SIGPOLL      = 29, /* I/O possible */
	SIGIO        = 29, /* Alias pour SIGPOLL */
	SIGPWR       = 30, /* Power failure */
	SIGSYS       = 31, /* Bad system call */
	SIG_Sentinel = 32  /* SIG_Sentinel */
};

#define MAX_SIG SIG_Sentinel

typedef typeof(void(int)) *sighandler_t;

#define SIG_ERR ((sighandler_t) - 1)

static inline const char *signal_to_string(enum signals sig)
{
	static const char *names[] = {
	    [SIGHUP] = "SIGHUP",       [SIGINT] = "SIGINT",       [SIGQUIT] = "SIGQUIT",
	    [SIGILL] = "SIGILL",       [SIGTRAP] = "SIGTRAP",     [SIGABRT] = "SIGABRT",
	    [SIGBUS] = "SIGBUS",       [SIGFPE] = "SIGFPE",       [SIGKILL] = "SIGKILL",
	    [SIGUSR1] = "SIGUSR1",     [SIGSEGV] = "SIGSEGV",     [SIGUSR2] = "SIGUSR2",
	    [SIGPIPE] = "SIGPIPE",     [SIGALRM] = "SIGALRM",     [SIGTERM] = "SIGTERM",
	    [SIGSTKFLT] = "SIGSTKFLT", [SIGCHLD] = "SIGCHLD",     [SIGCONT] = "SIGCONT",
	    [SIGSTOP] = "SIGSTOP",     [SIGTSTP] = "SIGTSTP",     [SIGTTIN] = "SIGTTIN",
	    [SIGTTOU] = "SIGTTOU",     [SIGURG] = "SIGURG",       [SIGXCPU] = "SIGXCPU",
	    [SIGXFSZ] = "SIGXFSZ",     [SIGVTALRM] = "SIGVTALRM", [SIGPROF] = "SIGPROF",
	    [SIGWINCH] = "SIGWINCH",   [SIGPOLL] = "SIGPOLL",     [SIGPWR] = "SIGPWR",
	    [SIGSYS] = "SIGSYS",
	};
	if (sig < 1 || sig >= SIG_Sentinel)
		return "UNKNOWN";
	return names[sig] ? names[sig] : "UNKNOWN";
}

struct sigframe {
	uint32_t          ret_addr;
	int               sig_num;
	struct trap_frame tf_backup;
} __attribute__((packed));

bool signal_is_valid(enum signals sig);
bool signal_check_perm(struct task *target);
void signal_send(enum signals sig, struct task *dst);
bool signal_is_set(enum signals sig, struct task *dst);
void signal_reset(struct task *dst);
void signal_dequeue(enum signals sig, struct task *dst);
int  signal_dequeue_yield(struct task *task);
void signal_init_default_handlers(struct task *task);
void signal_call_curtask_handlers(void);
