#include <proc/task.h>
#include <syscalls/syscalls.h>

SYSCALL_DEFINE0(getuid) { return task_get_current_task()->uid; }
