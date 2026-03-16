#pragma once

#include <arch/trap_frame.h>
#include <types.h>

#define MAX_SYSCALL 200
#define SYS_INT     0x80
#define asmlinkage  __attribute__((regparm(0)))

typedef asmlinkage long (*syscallHandler)(long, long, long, long, long);

extern const syscallHandler syscall_table[MAX_SYSCALL];

#define SYSCALL_DEFINE0(name) asmlinkage long sys_##name(void)

#define SYSCALL_DEFINE1(name, type1, arg1) asmlinkage long sys_##name(type1 arg1)

#define SYSCALL_DEFINE2(name, type1, arg1, type2, arg2)                                            \
	asmlinkage long sys_##name(type1 arg1, type2 arg2)

#define SYSCALL_DEFINE3(name, type1, arg1, type2, arg2, type3, arg3)                               \
	asmlinkage long sys_##name(type1 arg1, type2 arg2, type3 arg3)

#define SYSCALL_DEFINE4(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)                  \
	asmlinkage long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)

#define SYSCALL_DEFINE5(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)     \
	asmlinkage long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)

void do_syscall(struct trap_frame *tf);
