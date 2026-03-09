#pragma once

#include <types.h>

#define SYSCALL_DEFINE0(name)                                                                      \
	long sys_##name(void);                                                                         \
	long sys_##name(void)

#define SYSCALL_DEFINE1(name, type1, arg1)                                                         \
	long sys_##name(type1 arg1);                                                                   \
	long sys_##name(type1 arg1)

#define SYSCALL_DEFINE2(name, type1, arg1, type2, arg2)                                            \
	long sys_##name(type1 arg1, type2 arg2);                                                       \
	long sys_##name(type1 arg1, type2 arg2)

#define SYSCALL_DEFINE3(name, type1, arg1, type2, arg2, type3, arg3)                               \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3);                                           \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3)

#define SYSCALL_DEFINE4(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)                  \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4);                               \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)

#define SYSCALL_DEFINE5(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)     \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5);                   \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)
