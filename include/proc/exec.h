#pragma once

#include <proc/task.h>
#include <types.h>

extern void hello_kproc(void);

int  exec_fn(void *fn_start, size_t fn_size, const char *fn_name, bool userspace);
int  exec_mok(const char *name);
void exec_task(struct task *task, bool userspace);
