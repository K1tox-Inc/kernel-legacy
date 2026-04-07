#pragma once

#include <types.h>

struct task;

int  sched_enqueue(struct task *task);
void schedule(void);
