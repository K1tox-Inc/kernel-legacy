#pragma once

#include <arch/trap_frame.h>
#include <types.h>

#define MAX_SYSCALL 200
typedef long (*syscallHandler)(struct trap_frame *frame);
#define SYS_INT 0x80

extern void *syscall_table[MAX_SYSCALL + 1];
#include <arch/trap_frame.h>

#define SYSCALL_DEFINE0(name)                                                                      \
	long sys_##name(void);                                                                         \
	long wrap_sys_##name(struct trap_frame *tf)                                                    \
	{                                                                                              \
		(void)tf;                                                                                  \
		return sys_##name();                                                                       \
	}                                                                                              \
	long sys_##name(void)

#define SYSCALL_DEFINE1(name, type1, arg1)                                                         \
	long sys_##name(type1 arg1);                                                                   \
	long wrap_sys_##name(struct trap_frame *tf) { return sys_##name((type1)tf->regs.ebx); }        \
	long sys_##name(type1 arg1)

#define SYSCALL_DEFINE2(name, type1, arg1, type2, arg2)                                            \
	long sys_##name(type1 arg1, type2 arg2);                                                       \
	long wrap_sys_##name(struct trap_frame *tf)                                                    \
	{                                                                                              \
		return sys_##name((type1)tf->regs.ebx, (type2)tf->regs.ecx);                               \
	}                                                                                              \
	long sys_##name(type1 arg1, type2 arg2)

#define SYSCALL_DEFINE3(name, type1, arg1, type2, arg2, type3, arg3)                               \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3);                                           \
	long wrap_sys_##name(struct trap_frame *tf)                                                    \
	{                                                                                              \
		return sys_##name((type1)tf->regs.ebx, (type2)tf->regs.ecx, (type3)tf->regs.edx);          \
	}                                                                                              \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3)

#define SYSCALL_DEFINE4(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)                  \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4);                               \
	long wrap_sys_##name(struct trap_frame *tf)                                                    \
	{                                                                                              \
		return sys_##name((type1)tf->regs.ebx, (type2)tf->regs.ecx, (type3)tf->regs.edx,           \
		                  (type4)tf->regs.esi);                                                    \
	}                                                                                              \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)

#define SYSCALL_DEFINE5(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)     \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5);                   \
	long wrap_sys_##name(struct trap_frame *tf)                                                    \
	{                                                                                              \
		return sys_##name((type1)tf->regs.ebx, (type2)tf->regs.ecx, (type3)tf->regs.edx,           \
		                  (type4)tf->regs.esi, (type5)tf->regs.edi);                               \
	}                                                                                              \
	long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)

void do_syscall(struct trap_frame *tf);
