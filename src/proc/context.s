.intel_syntax noprefix
.code32

.extern g_tss

# tss + 4 => esp0
# task + 40 => cr3
# task + 44 => kernel_stack_ptr

# void switch_to(struct task_struct *prev, struct task_struct *next);
switch_to:

  ret
