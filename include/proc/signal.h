#pragma once

#include <types.h>

struct task;

enum signals {
	SIGHUP    = 1,  /* Hangup */
	SIGINT    = 2,  /* Interrupt (Ctrl+C) */
	SIGQUIT   = 3,  /* Quit */
	SIGILL    = 4,  /* Illegal instruction */
	SIGTRAP   = 5,  /* Trace/breakpoint trap */
	SIGABRT   = 6,  /* Aborted */
	SIGIOT    = 6,  /* Alias pour SIGABRT */
	SIGBUS    = 7,  /* Bus error */
	SIGFPE    = 8,  /* Floating point exception */
	SIGKILL   = 9,  /* Killed (force quit) */
	SIGUSR1   = 10, /* User defined signal 1 */
	SIGSEGV   = 11, /* Segmentation fault */
	SIGUSR2   = 12, /* User defined signal 2 */
	SIGPIPE   = 13, /* Broken pipe */
	SIGALRM   = 14, /* Alarm clock */
	SIGTERM   = 15, /* Terminated */
	SIGSTKFLT = 16, /* Stack fault */
	SIGCHLD   = 17, /* Child exited */
	SIGCLD    = 17, /* Alias pour SIGCHLD */
	SIGCONT   = 18, /* Continued */
	SIGSTOP   = 19, /* Stopped (signal) */
	SIGTSTP   = 20, /* Stopped (user) */
	SIGTTIN   = 21, /* Stopped (tty input) */
	SIGTTOU   = 22, /* Stopped (tty output) */
	SIGURG    = 23, /* Urgent I/O condition */
	SIGXCPU   = 24, /* CPU time limit exceeded */
	SIGXFSZ   = 25, /* File size limit exceeded */
	SIGVTALRM = 26, /* Virtual timer expired */
	SIGPROF   = 27, /* Profiling timer expired */
	SIGWINCH  = 28, /* Window changed */
	SIGPOLL   = 29, /* I/O possible */
	SIGIO     = 29, /* Alias pour SIGPOLL */
	SIGPWR    = 30, /* Power failure */
	SIGSYS    = 31, /* Bad system call */
	Sentinel  = 32  /* Sentinel */
};

bool signal_is_valid(enum signals sig);
bool signal_check_perm(struct task *target);
void signal_send(enum signals sig, struct task *dst);
bool signal_is_set(enum signals sig, struct task *dst);
void signal_reset(struct task *dst);
