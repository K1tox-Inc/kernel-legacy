.intel_syntax noprefix
.code32

.section .text

.extern g_tss
.extern current_task
.global switch_to
.global sig_trampoline_start
.global sig_trampoline_end

.macro SYSCALL_SIGRETURN
    mov eax, 119
    int 0x80
.endm

# =============================================================================
# STRUCT TASK LAYOUT (Memory Map)
# =============================================================================
# Offset | Size | Field
# -------|------|--------------------------------------------------------------
# 0      | 4    | pid_t pid
# 4      | 4    | uid_t uid
# 8      | 4    | gid_t gid
# 12     | 4    | struct task *real_parent
# 16     | 4    | struct task *parent
# 20     | 8    | struct list_head children (prev=20, next=24)
# 28     | 8    | struct list_head siblings (prev=28, next=32)
# 36     | 4    | uintptr_t esp (CPU Context Save)
# 40     | 4    | uintptr_t cr3 (Page Directory Pointer)
# 44     | 4    | uintptr_t kernel_stack_pointer (Bottom / Canary)
# 48     | 4    | uintptr_t kernel_stack_base (Top / TSS.esp0)
# ...    |      | (others fields not used here)
# =============================================================================

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

  mov eax, [ebx + 48]
  mov [g_tss + 4], eax

  pop edi
  pop esi
  pop ebx
  pop ebp

  ret

sig_trampoline_start:
  mov eax, 119
  int 0x80
sig_trampoline_end:

