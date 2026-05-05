#include <proc/task.h>
#include <syscalls/syscalls.h>

SYSCALL_DEFINE0(getpid) { return task_get_current_task()->pid; }
