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

  mov ebx, [ebp + 8]
  mov [ebx + 36], esp

  mov ebx, [ebp + 12]
  mov esp, [ebx + 36]

  mov eax, [ebx + 40]
  mov cr3, eax

  mov eax, [ebx + 44]
  mov [g_tss + 4], eax

  pop edi
  pop esi
  pop ebx

  mov esp, ebp
  pop ebp

  ret
