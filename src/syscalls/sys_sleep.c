#include <proc/timer.h>
#include <syscalls/syscalls.h>

SYSCALL_DEFINE1(sleep, int, seconds)
{
	timer_ksleep(seconds);
	return 0;
}
