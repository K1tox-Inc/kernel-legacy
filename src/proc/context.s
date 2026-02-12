.intel_syntax noprefix
.code32

.extern g_tss

# tss + 4 => esp0
# task + 36 => esp
# task + 40 => cr3
# task + 44 => kernel_stack_ptr

# void switch_to(struct task *current, struct task *next);
switch_to:
  push ebp
  mov ebp, esp

  push ebx
  push esi
  push edi

  pop edi
  pop esi
  pop ebx

  mov esp, ebp
  pop ebp

  ret
