#pragma once

#include <types.h>

extern void hello_kproc(void);

int exec_fn(void *fn_start, size_t fn_size, char *fn_name, bool userspace);
int exec_mok(const char *name);
int exec_task(struct task *task, bool userspace);
