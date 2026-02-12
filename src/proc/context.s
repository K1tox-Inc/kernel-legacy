.intel_syntax noprefix
.code32

.extern g_tss

# Include auto-generated offset constants
.include "include/proc/task_offsets.inc"

# void switch_to(struct task *current, struct task *next);
switch_to:
  push ebp
  mov ebp, esp

  push ebx
  push esi
  push edi

  mov ebx, [ebp + 8]
  mov [ebx + TASK_esp_OFFSET], esp

  mov ebx, [ebp + 12]
  mov esp, [ebx + TASK_esp_OFFSET]

  mov eax, [ebx + TASK_cr3_OFFSET]
  mov cr3, eax

  mov eax, [ebx + TASK_kernel_stack_pointer_OFFSET]
  mov [g_tss + TSS_ESP0_OFFSET], eax

  pop edi
  pop esi
  pop ebx

  mov esp, ebp
  pop ebp

  ret
