#include <libk.h>
#include <syscalls/syscalls.h>
#include <utils/error.h>

SYSCALL_DEFINE1(exit, int, status)
{

    return status;
}
